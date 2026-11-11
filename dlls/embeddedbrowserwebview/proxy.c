/*
 * Generic COM proxy runtime: marshalling, object tables, calls (docs/architektura-proxy.md).
 *
 * Copyright 2026 wine-webview2 contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdlib.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "shlwapi.h"
#include "webview2.h"
#include "webview2_private.h"
#include "protocol.h"
#include "proxy.h"
#include "proxy_gen.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/rbtree.h"

WINE_DEFAULT_DEBUG_CHANNEL(webview2);

static CRITICAL_SECTION proxy_cs;
static CRITICAL_SECTION_DEBUG proxy_cs_debug =
{
    0, 0, &proxy_cs,
    { &proxy_cs_debug.ProcessLocksList, &proxy_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": proxy_cs") }
};
static CRITICAL_SECTION proxy_cs = { &proxy_cs_debug, -1, 0, 0, 0, 0 };

/*
 * Buffers.
 */

static void buffer_reserve(struct wv2_buffer *buf, size_t size)
{
    size_t capacity;
    char *data;

    if (buf->size + size <= buf->capacity) return;
    capacity = max(buf->capacity * 2, buf->size + size + 64);
    if (!(data = realloc(buf->data, capacity))) return;
    buf->data = data;
    buf->capacity = capacity;
}

void wv2_put_bytes(struct wv2_buffer *buf, const void *data, size_t size)
{
    buffer_reserve(buf, size);
    if (buf->size + size > buf->capacity) return;
    memcpy(buf->data + buf->size, data, size);
    buf->size += size;
}

void wv2_put_u32(struct wv2_buffer *buf, UINT32 value)
{
    wv2_put_bytes(buf, &value, sizeof(value));
}

void wv2_put_u64(struct wv2_buffer *buf, UINT64 value)
{
    wv2_put_bytes(buf, &value, sizeof(value));
}

void wv2_put_f64(struct wv2_buffer *buf, double value)
{
    wv2_put_bytes(buf, &value, sizeof(value));
}

void wv2_put_str(struct wv2_buffer *buf, const WCHAR *str)
{
    int len;

    if (!str)
    {
        wv2_put_u32(buf, ~0u);
        return;
    }
    len = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL) - 1;
    wv2_put_u32(buf, len);
    buffer_reserve(buf, len);
    if (buf->size + len > buf->capacity) return;
    WideCharToMultiByte(CP_UTF8, 0, str, -1, buf->data + buf->size, len, NULL, NULL);
    buf->size += len;
}

void wv2_put_stream(struct wv2_buffer *buf, IStream *stream)
{
    size_t start;
    ULONG read;
    HRESULT hr;

    if (!stream)
    {
        wv2_put_u32(buf, ~0u);
        return;
    }
    /* Passed by value: the whole remaining content. */
    wv2_put_u32(buf, 0);
    start = buf->size;
    for (;;)
    {
        buffer_reserve(buf, 65536);
        if (buf->size + 65536 > buf->capacity) break;
        read = 0;
        hr = IStream_Read(stream, buf->data + buf->size, 65536, &read);
        if (FAILED(hr) || !read) break;
        buf->size += read;
        if (hr == S_FALSE) break;
    }
    *(UINT32 *)(buf->data + start - sizeof(UINT32)) = buf->size - start;
}

static const void *reader_take(struct wv2_reader *rd, size_t size)
{
    const void *ret;

    if (rd->size - rd->pos < size)
    {
        rd->pos = rd->size;
        return NULL;
    }
    ret = rd->data + rd->pos;
    rd->pos += size;
    return ret;
}

UINT32 wv2_get_u32(struct wv2_reader *rd)
{
    const UINT32 *p = reader_take(rd, sizeof(*p));
    return p ? *p : 0;
}

UINT64 wv2_get_u64(struct wv2_reader *rd)
{
    const UINT64 *p = reader_take(rd, sizeof(*p));
    return p ? *p : 0;
}

double wv2_get_f64(struct wv2_reader *rd)
{
    const double *p = reader_take(rd, sizeof(*p));
    return p ? *p : 0.0;
}

void wv2_get_bytes(struct wv2_reader *rd, void *data, size_t size)
{
    const void *p = reader_take(rd, size);
    if (p) memcpy(data, p, size);
    else memset(data, 0, size);
}

static WCHAR *get_str(struct wv2_reader *rd, void *(*alloc)(SIZE_T))
{
    UINT32 len = wv2_get_u32(rd);
    const char *utf8;
    WCHAR *ret;
    int size;

    if (len == ~0u) return NULL;
    if (!(utf8 = reader_take(rd, len))) len = 0;
    size = len ? MultiByteToWideChar(CP_UTF8, 0, utf8, len, NULL, 0) : 0;
    if (!(ret = alloc((size + 1) * sizeof(WCHAR)))) return NULL;
    if (size) MultiByteToWideChar(CP_UTF8, 0, utf8, len, ret, size);
    ret[size] = 0;
    return ret;
}

static void *malloc_size(SIZE_T size)
{
    return malloc(size);
}

static void *co_alloc(SIZE_T size)
{
    return CoTaskMemAlloc(size);   /* WINAPI: not usable as a plain function pointer on i386 */
}

WCHAR *wv2_get_str(struct wv2_reader *rd)
{
    return get_str(rd, malloc_size);
}

WCHAR *wv2_get_str_co(struct wv2_reader *rd)
{
    return get_str(rd, co_alloc);
}

IStream *wv2_get_stream(struct wv2_reader *rd)
{
    UINT32 len = wv2_get_u32(rd);
    const void *data;

    if (len == ~0u) return NULL;
    data = reader_take(rd, len);
    return SHCreateMemStream(data, data ? len : 0);
}

void wv2_reader_free(struct wv2_reader *rd)
{
    free(rd->owned);
    memset(rd, 0, sizeof(*rd));
}

/*
 * Proxies of runtime objects.
 */

struct proxy
{
    const void *vtbl;
    LONG ref;
    UINT32 family;
    UINT64 id;
    LONG remote_refs;               /* references received from the helper */
    void *host_data;
    void (*host_destroy)(void *data);
    struct rb_entry id_entry;
    struct rb_entry ptr_entry;
};

static const void *host_vtbls[WV2_FAMILY_COUNT];

static int compare_u64(const void *key, const struct rb_entry *entry, size_t offset)
{
    UINT64 a = *(const UINT64 *)key, b = *(const UINT64 *)((const char *)entry - offset);
    return a < b ? -1 : a > b;
}

static int proxy_id_compare(const void *key, const struct rb_entry *entry)
{
    return compare_u64(key, entry, offsetof(struct proxy, id_entry) - offsetof(struct proxy, id));
}

static int proxy_ptr_compare(const void *key, const struct rb_entry *entry)
{
    const void *a = key, *b = RB_ENTRY_VALUE(entry, const struct proxy, ptr_entry);
    return a < b ? -1 : a > b;
}

static struct rb_tree proxies_by_id = { proxy_id_compare };
static struct rb_tree proxies_by_ptr = { proxy_ptr_compare };

const void *proxy_get_vtbl(UINT32 family)
{
    return family < WV2_FAMILY_COUNT ? proxy_vtbls[family] : NULL;
}

void proxy_set_host_vtbl(UINT32 family, const void *vtbl)
{
    if (family < WV2_FAMILY_COUNT) host_vtbls[family] = vtbl;
}

BOOL proxy_is_proxy(IUnknown *iface)
{
    BOOL ret;

    EnterCriticalSection(&proxy_cs);
    ret = iface && rb_get(&proxies_by_ptr, iface) != NULL;
    LeaveCriticalSection(&proxy_cs);
    return ret;
}

void *proxy_get_host_data(IUnknown *iface)
{
    return ((struct proxy *)iface)->host_data;
}

void proxy_set_host_data(IUnknown *iface, void *data, void (*destroy)(void *data))
{
    struct proxy *proxy = (struct proxy *)iface;
    proxy->host_data = data;
    proxy->host_destroy = destroy;
}

HRESULT STDMETHODCALLTYPE proxy_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct proxy *proxy = (struct proxy *)iface;
    const IID *const *p;

    for (p = proxy_family_iids[proxy->family]; *p; p++)
    {
        if (IsEqualGUID(iid, *p))
        {
            proxy_AddRef(iface);
            *out = iface;
            return S_OK;
        }
    }
    TRACE("%s %p: unsupported interface %s\n", proxy_family_names[proxy->family], iface, debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE proxy_AddRef(IUnknown *iface)
{
    struct proxy *proxy = (struct proxy *)iface;
    return InterlockedIncrement(&proxy->ref);
}

ULONG STDMETHODCALLTYPE proxy_Release(IUnknown *iface)
{
    struct proxy *proxy = (struct proxy *)iface;
    struct wv2_release msg;
    ULONG ref = InterlockedDecrement(&proxy->ref);

    if (ref) return ref;
    EnterCriticalSection(&proxy_cs);
    /* get_proxy() may have found and revived it (under proxy_cs) before we got the lock. */
    if (ReadNoFence(&proxy->ref))
    {
        LeaveCriticalSection(&proxy_cs);
        return 0;
    }
    rb_remove(&proxies_by_id, &proxy->id_entry);
    rb_remove(&proxies_by_ptr, &proxy->ptr_entry);
    msg.count = proxy->remote_refs;
    LeaveCriticalSection(&proxy_cs);
    TRACE("releasing %s proxy %p (id %s)\n", proxy_family_names[proxy->family], iface, wine_dbgstr_longlong(proxy->id));
    msg.id = proxy->id;
    msg.reserved = 0;
    webview2_send(0, WV2_CMD_RELEASE, &msg, sizeof(msg));
    if (proxy->host_destroy) proxy->host_destroy(proxy->host_data);
    free(proxy);
    return 0;
}

/* Proxy for a received runtime reference (one more remote reference). */
static IUnknown *get_proxy(UINT32 family, UINT64 id)
{
    struct proxy *proxy;
    struct rb_entry *entry;

    if (family >= WV2_FAMILY_COUNT) return NULL;
    EnterCriticalSection(&proxy_cs);
    if ((entry = rb_get(&proxies_by_id, &id)))
    {
        proxy = RB_ENTRY_VALUE(entry, struct proxy, id_entry);
        InterlockedIncrement(&proxy->ref);
        proxy->remote_refs++;
        LeaveCriticalSection(&proxy_cs);
        return (IUnknown *)proxy;
    }
    if ((proxy = calloc(1, sizeof(*proxy))))
    {
        proxy->vtbl = host_vtbls[family] ? host_vtbls[family] : proxy_vtbls[family];
        proxy->ref = 1;
        proxy->family = family;
        proxy->id = id;
        proxy->remote_refs = 1;
        rb_put(&proxies_by_id, &proxy->id, &proxy->id_entry);
        rb_put(&proxies_by_ptr, proxy, &proxy->ptr_entry);
        TRACE("new %s proxy %p for id %s\n", proxy_family_names[family], proxy, wine_dbgstr_longlong(id));
    }
    LeaveCriticalSection(&proxy_cs);
    return (IUnknown *)proxy;
}

/*
 * Application objects referenced by the helper (handlers, options, ...).
 */

struct app_object
{
    UINT64 id;
    IUnknown *obj;
    HWND home;                      /* async window of the thread that passed the object */
    LONG sent;                      /* references sent to the helper */
    LONG pending_invokes;           /* invoke jobs queued but not run yet */
    LONG deferred_release;          /* release counts waiting for those jobs */
    struct rb_entry id_entry;
    struct rb_entry ptr_entry;
};

static int app_id_compare(const void *key, const struct rb_entry *entry)
{
    return compare_u64(key, entry, offsetof(struct app_object, id_entry) - offsetof(struct app_object, id));
}

static int app_ptr_compare(const void *key, const struct rb_entry *entry)
{
    const void *a = key, *b = RB_ENTRY_VALUE(entry, const struct app_object, ptr_entry)->obj;
    return a < b ? -1 : a > b;
}

static struct rb_tree apps_by_id = { app_id_compare };
static struct rb_tree apps_by_ptr = { app_ptr_compare };
static UINT64 next_app_id = 1;

void wv2_put_iface(struct wv2_buffer *buf, IUnknown *obj)
{
    struct wv2_objref ref = {WV2_OBJREF_NULL};
    struct app_object *app;
    struct rb_entry *entry;

    if (obj)
    {
        EnterCriticalSection(&proxy_cs);
        if ((entry = rb_get(&proxies_by_ptr, obj)))
        {
            struct proxy *proxy = RB_ENTRY_VALUE(entry, struct proxy, ptr_entry);
            ref.kind = WV2_OBJREF_RUNTIME;
            ref.family = proxy->family;
            ref.id = proxy->id;
        }
        else
        {
            if ((entry = rb_get(&apps_by_ptr, obj))) app = RB_ENTRY_VALUE(entry, struct app_object, ptr_entry);
            else if ((app = calloc(1, sizeof(*app))))
            {
                app->id = next_app_id++;
                app->obj = obj;
                app->home = webview2_get_async_window();
                IUnknown_AddRef(obj);
                rb_put(&apps_by_id, &app->id, &app->id_entry);
                rb_put(&apps_by_ptr, obj, &app->ptr_entry);
            }
            if (app)
            {
                app->sent++;
                ref.kind = WV2_OBJREF_APP;
                ref.id = app->id;
            }
        }
        LeaveCriticalSection(&proxy_cs);
    }
    wv2_put_bytes(buf, &ref, sizeof(ref));
}

IUnknown *wv2_get_iface(struct wv2_reader *rd)
{
    struct wv2_objref ref;
    struct rb_entry *entry;
    IUnknown *ret = NULL;

    wv2_get_bytes(rd, &ref, sizeof(ref));
    switch (ref.kind)
    {
    case WV2_OBJREF_RUNTIME:
        return get_proxy(ref.family, ref.id);
    case WV2_OBJREF_APP:
        EnterCriticalSection(&proxy_cs);
        if ((entry = rb_get(&apps_by_id, &ref.id)))
        {
            ret = RB_ENTRY_VALUE(entry, struct app_object, id_entry)->obj;
            IUnknown_AddRef(ret);
        }
        LeaveCriticalSection(&proxy_cs);
        return ret;
    }
    return NULL;
}

/*
 * Calls into the helper.
 */

struct pending_call
{
    struct list entry;
    UINT64 call_id;
    HANDLE event;
    HRESULT hr;
    char *reply;
    UINT32 reply_size;
};

static struct list pending_calls = LIST_INIT(pending_calls);
static BOOL connection_lost;

static HRESULT call(UINT64 object, UINT32 method, struct wv2_buffer *args, struct wv2_reader *ret)
{
    static LONG64 next_call_id;
    struct pending_call pending = {{0}};
    struct wv2_call header;
    struct wv2_buffer msg = {0};
    HRESULT hr;

    memset(ret, 0, sizeof(*ret));
    header.call_id = pending.call_id = InterlockedIncrement64(&next_call_id);
    header.object = object;
    header.method = method;
    header.flags = 0;
    wv2_put_bytes(&msg, &header, sizeof(header));
    if (args->size) wv2_put_bytes(&msg, args->data, args->size);
    free(args->data);
    memset(args, 0, sizeof(*args));
    if (msg.size < sizeof(header))
    {
        free(msg.data);
        return E_OUTOFMEMORY;
    }

    pending.event = CreateEventW(NULL, TRUE, FALSE, NULL);
    EnterCriticalSection(&proxy_cs);
    if (connection_lost)
    {
        LeaveCriticalSection(&proxy_cs);
        CloseHandle(pending.event);
        free(msg.data);
        return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    }
    list_add_tail(&pending_calls, &pending.entry);
    LeaveCriticalSection(&proxy_cs);

    TRACE("call %s: object %s, method %u, %Iu bytes\n", wine_dbgstr_longlong(header.call_id),
          wine_dbgstr_longlong(object), method, msg.size - sizeof(header));
    if (FAILED(hr = webview2_send(0, WV2_CMD_CALL, msg.data, msg.size)))
    {
        EnterCriticalSection(&proxy_cs);
        list_remove(&pending.entry);
        LeaveCriticalSection(&proxy_cs);
    }
    else
    {
        /* The helper never sends a synchronous INVOKE while handling a CALL, so plain waiting
         * cannot deadlock; INVOKEs queued meanwhile run once we return to the message loop. */
        WaitForSingleObject(pending.event, INFINITE);
        hr = pending.hr;
        TRACE("call %s done: %#lx\n", wine_dbgstr_longlong(header.call_id), hr);
        ret->data = ret->owned = pending.reply;
        ret->size = pending.reply_size;
    }
    CloseHandle(pending.event);
    free(msg.data);
    return hr;
}

HRESULT proxy_call(IUnknown *iface, UINT32 method, struct wv2_buffer *args, struct wv2_reader *ret)
{
    return call(((struct proxy *)iface)->id, method, args, ret);
}

HRESULT proxy_call_root(UINT32 method, struct wv2_buffer *args, struct wv2_reader *ret)
{
    return call(0, method, args, ret);
}

void proxy_handle_reply(const BYTE *payload, UINT32 size)
{
    const struct wv2_reply *reply = (const void *)payload;
    struct pending_call *pending;

    if (size < sizeof(*reply)) return;
    EnterCriticalSection(&proxy_cs);
    LIST_FOR_EACH_ENTRY(pending, &pending_calls, struct pending_call, entry)
    {
        if (pending->call_id != reply->call_id) continue;
        list_remove(&pending->entry);
        pending->hr = reply->hresult;
        pending->reply_size = size - sizeof(*reply);
        if ((pending->reply = malloc(max(1, pending->reply_size))))
            memcpy(pending->reply, reply + 1, pending->reply_size);
        SetEvent(pending->event);
        break;
    }
    LeaveCriticalSection(&proxy_cs);
}

void proxy_connection_lost(void)
{
    struct pending_call *pending, *next;

    EnterCriticalSection(&proxy_cs);
    connection_lost = TRUE;
    LIST_FOR_EACH_ENTRY_SAFE(pending, next, &pending_calls, struct pending_call, entry)
    {
        list_remove(&pending->entry);
        pending->hr = HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
        SetEvent(pending->event);
    }
    LeaveCriticalSection(&proxy_cs);
}

/*
 * Calls from the helper on application objects, run on the object's home thread.
 */

struct invoke_job
{
    struct wv2_call call;
    UINT64 app_id;
    IUnknown *obj;
    UINT32 size;
    BYTE args[];
};

/* Applies a release from the helper; returns the object to Release when it is gone (proxy_cs held). */
static IUnknown *apply_app_release(struct app_object *app, LONG count)
{
    IUnknown *obj;

    if ((app->sent -= count) > 0) return NULL;
    rb_remove(&apps_by_id, &app->id_entry);
    rb_remove(&apps_by_ptr, &app->ptr_entry);
    obj = app->obj;
    free(app);
    return obj;
}

static void invoke_cb(void *arg)
{
    struct invoke_job *job = arg;
    struct wv2_reader args = {(const char *)job->args, job->size};
    struct wv2_buffer out = {0};
    struct wv2_reply reply = {job->call.call_id};
    HRESULT hr;

    struct rb_entry *entry;
    IUnknown *released = NULL;

    wv2_put_bytes(&out, &reply, sizeof(reply));
    hr = proxy_invoke_app(job->obj, job->call.method, &args, &out);
    TRACE("invoke %u on %p returned %#lx\n", job->call.method, job->obj, hr);
    if (!(job->call.flags & WV2_CALL_ASYNC) && out.size >= sizeof(reply))
    {
        ((struct wv2_reply *)out.data)->hresult = hr;
        webview2_send(0, WV2_CMD_INVOKE_REPLY, out.data, out.size);
    }
    free(out.data);

    /* Like the real runtime, drop our reference right after the last invoke, in the same step:
     * applications (e.g. webview/webview) keep handlers on the stack while waiting for them. */
    EnterCriticalSection(&proxy_cs);
    if ((entry = rb_get(&apps_by_id, &job->app_id)))
    {
        struct app_object *app = RB_ENTRY_VALUE(entry, struct app_object, id_entry);
        if (!--app->pending_invokes && app->deferred_release)
        {
            LONG count = app->deferred_release;
            app->deferred_release = 0;
            released = apply_app_release(app, count);
        }
    }
    LeaveCriticalSection(&proxy_cs);
    IUnknown_Release(job->obj);
    if (released) IUnknown_Release(released);
    free(job);
}

void proxy_handle_invoke(const BYTE *payload, UINT32 size)
{
    const struct wv2_call *call = (const void *)payload;
    struct invoke_job *job;
    struct rb_entry *entry;
    IUnknown *obj = NULL;
    HWND home = NULL;

    if (size < sizeof(*call)) return;
    TRACE("invoke %s: app %s, method %u, flags %#x\n", wine_dbgstr_longlong(call->call_id),
          wine_dbgstr_longlong(call->object), call->method, call->flags);
    EnterCriticalSection(&proxy_cs);
    if ((entry = rb_get(&apps_by_id, &call->object)))
    {
        struct app_object *app = RB_ENTRY_VALUE(entry, struct app_object, id_entry);
        obj = app->obj;
        home = app->home;
        app->pending_invokes++;
        IUnknown_AddRef(obj);
    }
    LeaveCriticalSection(&proxy_cs);

    if (!obj || !(job = malloc(offsetof(struct invoke_job, args[size - sizeof(*call)]))))
    {
        struct wv2_reply reply = {call->call_id, E_FAIL};
        WARN("invoke on unknown application object %s\n", wine_dbgstr_longlong(call->object));
        if (obj) IUnknown_Release(obj);
        if (!(call->flags & WV2_CALL_ASYNC)) webview2_send(0, WV2_CMD_INVOKE_REPLY, &reply, sizeof(reply));
        return;
    }
    job->call = *call;
    job->app_id = call->object;
    job->obj = obj;
    job->size = size - sizeof(*call);
    memcpy(job->args, call + 1, job->size);
    if (!webview2_queue_async_to(home, invoke_cb, job))
    {
        WARN("cannot queue invoke, running it on the reader thread\n");
        invoke_cb(job);
    }
}

struct release_job
{
    UINT64 id;
    UINT32 count;
};

static void release_app_cb(void *arg)
{
    struct release_job *job = arg;
    struct rb_entry *entry;
    IUnknown *obj = NULL;

    EnterCriticalSection(&proxy_cs);
    if ((entry = rb_get(&apps_by_id, &job->id)))
        obj = apply_app_release(RB_ENTRY_VALUE(entry, struct app_object, id_entry), job->count);
    LeaveCriticalSection(&proxy_cs);
    if (obj) IUnknown_Release(obj);
    free(job);
}

void proxy_handle_release_app(const BYTE *payload, UINT32 size)
{
    const struct wv2_release *release = (const void *)payload;
    struct release_job *job;
    struct rb_entry *entry;
    HWND home = NULL;

    if (size < sizeof(*release)) return;
    EnterCriticalSection(&proxy_cs);
    if ((entry = rb_get(&apps_by_id, &release->id)))
    {
        struct app_object *app = RB_ENTRY_VALUE(entry, struct app_object, id_entry);
        if (app->pending_invokes)
        {
            /* Applied right after the queued invoke(s) run, see invoke_cb(). */
            app->deferred_release += release->count;
            LeaveCriticalSection(&proxy_cs);
            return;
        }
        home = app->home;
    }
    LeaveCriticalSection(&proxy_cs);
    if (!(job = malloc(sizeof(*job)))) return;
    job->id = release->id;
    job->count = release->count;
    if (!webview2_queue_async_to(home, release_app_cb, job)) release_app_cb(job);
}
