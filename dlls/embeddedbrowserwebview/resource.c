/*
 * WebResourceRequested: request/response/header objects and event dispatch.
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
#include "stubs.h"
#include "webview2_private.h"
#include "protocol.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(webview2);

#define CONTENT_CHUNK (1024 * 1024)  /* streaming granularity; large messages go through a memfd */

static WCHAR *strdupW(const WCHAR *str)
{
    size_t size;
    WCHAR *ret;

    if (!str) str = L"";
    size = (wcslen(str) + 1) * sizeof(WCHAR);
    if ((ret = malloc(size))) memcpy(ret, str, size);
    return ret;
}

static WCHAR *co_strdupW(const WCHAR *str)
{
    size_t size;
    WCHAR *ret;

    if (!str) str = L"";
    size = (wcslen(str) + 1) * sizeof(WCHAR);
    if ((ret = CoTaskMemAlloc(size))) memcpy(ret, str, size);
    return ret;
}

static WCHAR *strdupUtf8(const char *str, int len)
{
    int size = MultiByteToWideChar(CP_UTF8, 0, str, len, NULL, 0);
    WCHAR *ret;

    if (!(ret = malloc((size + 1) * sizeof(WCHAR)))) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, str, len, ret, size);
    ret[size] = 0;
    return ret;
}

/* Appends str as UTF-8 to a growing buffer. */
static BOOL buffer_append(char **buf, size_t *size, size_t *capacity, const WCHAR *str)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL) - 1;
    char *tmp;

    if (len <= 0) return TRUE;
    if (*size + len > *capacity)
    {
        size_t capacity_new = max(*capacity * 2, *size + len + 256);
        if (!(tmp = realloc(*buf, capacity_new))) return FALSE;
        *buf = tmp;
        *capacity = capacity_new;
    }
    WideCharToMultiByte(CP_UTF8, 0, str, -1, *buf + *size, len, NULL, NULL);
    *size += len;
    return TRUE;
}

/*
 * Header collection shared by the request/response header objects and their iterators.
 */

struct header
{
    WCHAR *name;
    WCHAR *value;
};

struct headers
{
    LONG ref;
    struct header *entries;
    unsigned int count, capacity;
};

static struct headers *headers_create(void)
{
    struct headers *headers = calloc(1, sizeof(*headers));
    if (headers) headers->ref = 1;
    return headers;
}

static void headers_addref(struct headers *headers)
{
    InterlockedIncrement(&headers->ref);
}

static void headers_release(struct headers *headers)
{
    unsigned int i;

    if (InterlockedDecrement(&headers->ref)) return;
    for (i = 0; i < headers->count; i++)
    {
        free(headers->entries[i].name);
        free(headers->entries[i].value);
    }
    free(headers->entries);
    free(headers);
}

static HRESULT headers_append(struct headers *headers, const WCHAR *name, const WCHAR *value)
{
    struct header *entries;

    if (headers->count == headers->capacity)
    {
        unsigned int capacity = max(8, headers->capacity * 2);
        if (!(entries = realloc(headers->entries, capacity * sizeof(*entries)))) return E_OUTOFMEMORY;
        headers->entries = entries;
        headers->capacity = capacity;
    }
    if (!(headers->entries[headers->count].name = strdupW(name))) return E_OUTOFMEMORY;
    if (!(headers->entries[headers->count].value = strdupW(value)))
    {
        free(headers->entries[headers->count].name);
        return E_OUTOFMEMORY;
    }
    headers->count++;
    return S_OK;
}

static int headers_find(struct headers *headers, const WCHAR *name, unsigned int start)
{
    unsigned int i;

    for (i = start; i < headers->count; i++)
        if (!wcsicmp(headers->entries[i].name, name)) return i;
    return -1;
}

static void headers_remove(struct headers *headers, const WCHAR *name)
{
    int i;

    while ((i = headers_find(headers, name, 0)) >= 0)
    {
        free(headers->entries[i].name);
        free(headers->entries[i].value);
        memmove(&headers->entries[i], &headers->entries[i + 1], (headers->count - i - 1) * sizeof(*headers->entries));
        headers->count--;
    }
}

/* Parses "Name: value" lines separated by CRLF or LF. */
static void headers_parse(struct headers *headers, const WCHAR *text)
{
    WCHAR *copy, *line, *next, *colon, *name, *value, *end;

    if (!text || !(copy = strdupW(text))) return;
    for (line = copy; line && *line; line = next)
    {
        if ((next = wcschr(line, '\n'))) *next++ = 0;
        if ((end = wcschr(line, '\r'))) *end = 0;
        if (!(colon = wcschr(line, ':'))) continue;
        *colon = 0;
        name = line;
        value = colon + 1;
        while (*name == ' ' || *name == '\t') name++;
        while (*value == ' ' || *value == '\t') value++;
        for (end = colon; end > name && (end[-1] == ' ' || end[-1] == '\t'); end--) end[-1] = 0;
        if (*name) headers_append(headers, name, value);
    }
    free(copy);
}

/* ICoreWebView2HttpHeadersCollectionIterator */

struct header_iterator
{
    struct headers *headers;
    WCHAR *name;            /* only entries with this name (GetHeaders), or NULL */
    int pos;
};

static ICoreWebView2HttpHeadersCollectionIteratorVtbl iterator_vtbl;
static struct stub_family iterator_family;

static struct header_iterator *iterator_from_iface(ICoreWebView2HttpHeadersCollectionIterator *iface)
{
    return ((struct stub_object *)iface)->data;
}

static void iterator_destroy(struct stub_object *obj)
{
    struct header_iterator *it = obj->data;

    headers_release(it->headers);
    free(it->name);
    free(it);
}

static int iterator_next(struct header_iterator *it, int start)
{
    if (it->name) return headers_find(it->headers, it->name, start);
    return start < it->headers->count ? start : -1;
}

static HRESULT create_iterator(struct headers *headers, const WCHAR *name,
                               ICoreWebView2HttpHeadersCollectionIterator **out)
{
    struct header_iterator *it;
    struct stub_object *obj;

    if (!out) return E_POINTER;
    if (!(it = calloc(1, sizeof(*it)))) return E_OUTOFMEMORY;
    if (name && !(it->name = strdupW(name)))
    {
        free(it);
        return E_OUTOFMEMORY;
    }
    if (!(obj = stub_create(&iterator_family)))
    {
        free(it->name);
        free(it);
        return E_OUTOFMEMORY;
    }
    headers_addref(headers);
    it->headers = headers;
    it->pos = iterator_next(it, 0);
    obj->data = it;
    obj->destroy = iterator_destroy;
    *out = (ICoreWebView2HttpHeadersCollectionIterator *)obj;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE iterator_GetCurrentHeader(ICoreWebView2HttpHeadersCollectionIterator *iface,
        LPWSTR *name, LPWSTR *value)
{
    struct header_iterator *it = iterator_from_iface(iface);

    TRACE("iface %p, name %p, value %p.\n", iface, name, value);
    if (!name || !value) return E_POINTER;
    if (it->pos < 0) return HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
    *name = co_strdupW(it->headers->entries[it->pos].name);
    *value = co_strdupW(it->headers->entries[it->pos].value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE iterator_get_HasCurrentHeader(ICoreWebView2HttpHeadersCollectionIterator *iface,
        BOOL *has)
{
    TRACE("iface %p, has %p.\n", iface, has);
    if (!has) return E_POINTER;
    *has = iterator_from_iface(iface)->pos >= 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE iterator_MoveNext(ICoreWebView2HttpHeadersCollectionIterator *iface, BOOL *has_next)
{
    struct header_iterator *it = iterator_from_iface(iface);

    TRACE("iface %p, has_next %p.\n", iface, has_next);
    if (!has_next) return E_POINTER;
    if (it->pos >= 0) it->pos = iterator_next(it, it->pos + 1);
    *has_next = it->pos >= 0;
    return S_OK;
}

/* ICoreWebView2HttpRequestHeaders / ICoreWebView2HttpResponseHeaders share the layout of the
 * methods we implement, but have separate vtables. */

static ICoreWebView2HttpRequestHeadersVtbl request_headers_vtbl;
static struct stub_family request_headers_family;
static ICoreWebView2HttpResponseHeadersVtbl response_headers_vtbl;
static struct stub_family response_headers_family;

static struct headers *headers_from_obj(void *iface)
{
    return ((struct stub_object *)iface)->data;
}

static void headers_obj_destroy(struct stub_object *obj)
{
    headers_release(obj->data);
}

static HRESULT create_headers_obj(const struct stub_family *family, struct headers *headers, void **out)
{
    struct stub_object *obj;

    if (!out) return E_POINTER;
    if (!(obj = stub_create(family))) return E_OUTOFMEMORY;
    headers_addref(headers);
    obj->data = headers;
    obj->destroy = headers_obj_destroy;
    *out = obj;
    return S_OK;
}

static HRESULT get_header(struct headers *headers, LPCWSTR name, LPWSTR *value)
{
    int i;

    if (!name || !value) return E_POINTER;
    if ((i = headers_find(headers, name, 0)) < 0)
    {
        *value = NULL;
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
    return (*value = co_strdupW(headers->entries[i].value)) ? S_OK : E_OUTOFMEMORY;
}

static HRESULT contains_header(struct headers *headers, LPCWSTR name, BOOL *contains)
{
    if (!name || !contains) return E_POINTER;
    *contains = headers_find(headers, name, 0) >= 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE request_headers_GetHeader(ICoreWebView2HttpRequestHeaders *iface, LPCWSTR name, LPWSTR *value)
{
    TRACE("iface %p, name %s, value %p.\n", iface, debugstr_w(name), value);
    return get_header(headers_from_obj(iface), name, value);
}

static HRESULT STDMETHODCALLTYPE request_headers_GetHeaders(ICoreWebView2HttpRequestHeaders *iface, LPCWSTR name,
        ICoreWebView2HttpHeadersCollectionIterator **iterator)
{
    TRACE("iface %p, name %s, iterator %p.\n", iface, debugstr_w(name), iterator);
    return create_iterator(headers_from_obj(iface), name, iterator);
}

static HRESULT STDMETHODCALLTYPE request_headers_Contains(ICoreWebView2HttpRequestHeaders *iface, LPCWSTR name, BOOL *contains)
{
    TRACE("iface %p, name %s, contains %p.\n", iface, debugstr_w(name), contains);
    return contains_header(headers_from_obj(iface), name, contains);
}

static HRESULT STDMETHODCALLTYPE request_headers_SetHeader(ICoreWebView2HttpRequestHeaders *iface, LPCWSTR name, LPCWSTR value)
{
    struct headers *headers = headers_from_obj(iface);

    FIXME("iface %p, name %s, value %s: not sent to the network yet.\n", iface, debugstr_w(name), debugstr_w(value));
    if (!name || !value) return E_INVALIDARG;
    headers_remove(headers, name);
    return headers_append(headers, name, value);
}

static HRESULT STDMETHODCALLTYPE request_headers_RemoveHeader(ICoreWebView2HttpRequestHeaders *iface, LPCWSTR name)
{
    FIXME("iface %p, name %s: not applied to the network request yet.\n", iface, debugstr_w(name));
    if (!name) return E_INVALIDARG;
    headers_remove(headers_from_obj(iface), name);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE request_headers_GetIterator(ICoreWebView2HttpRequestHeaders *iface,
        ICoreWebView2HttpHeadersCollectionIterator **iterator)
{
    TRACE("iface %p, iterator %p.\n", iface, iterator);
    return create_iterator(headers_from_obj(iface), NULL, iterator);
}

static HRESULT STDMETHODCALLTYPE response_headers_AppendHeader(ICoreWebView2HttpResponseHeaders *iface, LPCWSTR name, LPCWSTR value)
{
    TRACE("iface %p, name %s, value %s.\n", iface, debugstr_w(name), debugstr_w(value));
    if (!name || !value) return E_INVALIDARG;
    return headers_append(headers_from_obj(iface), name, value);
}

static HRESULT STDMETHODCALLTYPE response_headers_Contains(ICoreWebView2HttpResponseHeaders *iface, LPCWSTR name, BOOL *contains)
{
    TRACE("iface %p, name %s, contains %p.\n", iface, debugstr_w(name), contains);
    return contains_header(headers_from_obj(iface), name, contains);
}

static HRESULT STDMETHODCALLTYPE response_headers_GetHeader(ICoreWebView2HttpResponseHeaders *iface, LPCWSTR name, LPWSTR *value)
{
    TRACE("iface %p, name %s, value %p.\n", iface, debugstr_w(name), value);
    return get_header(headers_from_obj(iface), name, value);
}

static HRESULT STDMETHODCALLTYPE response_headers_GetHeaders(ICoreWebView2HttpResponseHeaders *iface, LPCWSTR name,
        ICoreWebView2HttpHeadersCollectionIterator **iterator)
{
    TRACE("iface %p, name %s, iterator %p.\n", iface, debugstr_w(name), iterator);
    return create_iterator(headers_from_obj(iface), name, iterator);
}

static HRESULT STDMETHODCALLTYPE response_headers_GetIterator(ICoreWebView2HttpResponseHeaders *iface,
        ICoreWebView2HttpHeadersCollectionIterator **iterator)
{
    TRACE("iface %p, iterator %p.\n", iface, iterator);
    return create_iterator(headers_from_obj(iface), NULL, iterator);
}

/*
 * ICoreWebView2WebResourceRequest
 */

struct request
{
    WCHAR *uri;
    WCHAR *method;
    struct headers *headers;
    IStream *content;
};

static ICoreWebView2WebResourceRequestVtbl request_vtbl;
static struct stub_family request_family;

static struct request *request_from_iface(ICoreWebView2WebResourceRequest *iface)
{
    return ((struct stub_object *)iface)->data;
}

static void request_destroy(struct stub_object *obj)
{
    struct request *req = obj->data;

    free(req->uri);
    free(req->method);
    if (req->headers) headers_release(req->headers);
    if (req->content) IStream_Release(req->content);
    free(req);
}

static HRESULT STDMETHODCALLTYPE request_get_Uri(ICoreWebView2WebResourceRequest *iface, LPWSTR *uri)
{
    TRACE("iface %p, uri %p.\n", iface, uri);
    if (!uri) return E_POINTER;
    return (*uri = co_strdupW(request_from_iface(iface)->uri)) ? S_OK : E_OUTOFMEMORY;
}

static HRESULT STDMETHODCALLTYPE request_put_Uri(ICoreWebView2WebResourceRequest *iface, LPCWSTR uri)
{
    struct request *req = request_from_iface(iface);
    WCHAR *copy;

    FIXME("iface %p, uri %s: redirecting requests is not implemented.\n", iface, debugstr_w(uri));
    if (!uri) return E_INVALIDARG;
    if (!(copy = strdupW(uri))) return E_OUTOFMEMORY;
    free(req->uri);
    req->uri = copy;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE request_get_Method(ICoreWebView2WebResourceRequest *iface, LPWSTR *method)
{
    TRACE("iface %p, method %p.\n", iface, method);
    if (!method) return E_POINTER;
    return (*method = co_strdupW(request_from_iface(iface)->method)) ? S_OK : E_OUTOFMEMORY;
}

static HRESULT STDMETHODCALLTYPE request_put_Method(ICoreWebView2WebResourceRequest *iface, LPCWSTR method)
{
    struct request *req = request_from_iface(iface);
    WCHAR *copy;

    FIXME("iface %p, method %s: changing requests is not implemented.\n", iface, debugstr_w(method));
    if (!method) return E_INVALIDARG;
    if (!(copy = strdupW(method))) return E_OUTOFMEMORY;
    free(req->method);
    req->method = copy;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE request_get_Content(ICoreWebView2WebResourceRequest *iface, IStream **content)
{
    struct request *req = request_from_iface(iface);

    TRACE("iface %p, content %p.\n", iface, content);
    if (!content) return E_POINTER;
    if ((*content = req->content)) IStream_AddRef(*content);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE request_put_Content(ICoreWebView2WebResourceRequest *iface, IStream *content)
{
    struct request *req = request_from_iface(iface);

    FIXME("iface %p, content %p: changing requests is not implemented.\n", iface, content);
    if (content) IStream_AddRef(content);
    if (req->content) IStream_Release(req->content);
    req->content = content;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE request_get_Headers(ICoreWebView2WebResourceRequest *iface,
        ICoreWebView2HttpRequestHeaders **headers)
{
    TRACE("iface %p, headers %p.\n", iface, headers);
    return create_headers_obj(&request_headers_family, request_from_iface(iface)->headers, (void **)headers);
}

/*
 * ICoreWebView2WebResourceResponse
 */

struct response
{
    IStream *content;
    int status;
    WCHAR *reason;
    struct headers *headers;
};

static ICoreWebView2WebResourceResponseVtbl response_vtbl;
static struct stub_family response_family;

static struct response *response_from_iface(ICoreWebView2WebResourceResponse *iface)
{
    return ((struct stub_object *)iface)->data;
}

static void response_destroy(struct stub_object *obj)
{
    struct response *resp = obj->data;

    if (resp->content) IStream_Release(resp->content);
    free(resp->reason);
    if (resp->headers) headers_release(resp->headers);
    free(resp);
}

static HRESULT STDMETHODCALLTYPE response_get_Content(ICoreWebView2WebResourceResponse *iface, IStream **content)
{
    struct response *resp = response_from_iface(iface);

    TRACE("iface %p, content %p.\n", iface, content);
    if (!content) return E_POINTER;
    if ((*content = resp->content)) IStream_AddRef(*content);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE response_put_Content(ICoreWebView2WebResourceResponse *iface, IStream *content)
{
    struct response *resp = response_from_iface(iface);

    TRACE("iface %p, content %p.\n", iface, content);
    if (content) IStream_AddRef(content);
    if (resp->content) IStream_Release(resp->content);
    resp->content = content;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE response_get_Headers(ICoreWebView2WebResourceResponse *iface,
        ICoreWebView2HttpResponseHeaders **headers)
{
    TRACE("iface %p, headers %p.\n", iface, headers);
    return create_headers_obj(&response_headers_family, response_from_iface(iface)->headers, (void **)headers);
}

static HRESULT STDMETHODCALLTYPE response_get_StatusCode(ICoreWebView2WebResourceResponse *iface, int *status)
{
    TRACE("iface %p, status %p.\n", iface, status);
    if (!status) return E_POINTER;
    *status = response_from_iface(iface)->status;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE response_put_StatusCode(ICoreWebView2WebResourceResponse *iface, int status)
{
    TRACE("iface %p, status %d.\n", iface, status);
    response_from_iface(iface)->status = status;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE response_get_ReasonPhrase(ICoreWebView2WebResourceResponse *iface, LPWSTR *reason)
{
    TRACE("iface %p, reason %p.\n", iface, reason);
    if (!reason) return E_POINTER;
    return (*reason = co_strdupW(response_from_iface(iface)->reason)) ? S_OK : E_OUTOFMEMORY;
}

static HRESULT STDMETHODCALLTYPE response_put_ReasonPhrase(ICoreWebView2WebResourceResponse *iface, LPCWSTR reason)
{
    struct response *resp = response_from_iface(iface);
    WCHAR *copy;

    TRACE("iface %p, reason %s.\n", iface, debugstr_w(reason));
    if (!(copy = strdupW(reason))) return E_OUTOFMEMORY;
    free(resp->reason);
    resp->reason = copy;
    return S_OK;
}

HRESULT resource_create_response(IStream *content, int status, LPCWSTR reason, LPCWSTR headers_text,
                                 ICoreWebView2WebResourceResponse **out)
{
    struct stub_object *obj;
    struct response *resp;

    TRACE("content %p, status %d, reason %s, headers %s.\n", content, status, debugstr_w(reason), debugstr_w(headers_text));
    if (!out) return E_POINTER;
    if (!(resp = calloc(1, sizeof(*resp)))) return E_OUTOFMEMORY;
    resp->status = status;
    resp->reason = strdupW(reason);
    resp->headers = headers_create();
    if (!resp->reason || !resp->headers || !(obj = stub_create(&response_family)))
    {
        free(resp->reason);
        if (resp->headers) headers_release(resp->headers);
        free(resp);
        return E_OUTOFMEMORY;
    }
    headers_parse(resp->headers, headers_text);
    if ((resp->content = content)) IStream_AddRef(content);
    obj->data = resp;
    obj->destroy = response_destroy;
    *out = (ICoreWebView2WebResourceResponse *)obj;
    return S_OK;
}

/*
 * One intercepted request: event args + deferral.
 */

struct resource_event
{
    LONG ref;
    LONG finished;
    UINT32 view;
    UINT64 id;
    UINT32 context;
    BOOL deferred;          /* GetDeferral() was called */
    BOOL dispatching;       /* handlers are still running */
    ICoreWebView2WebResourceRequest *request;
    ICoreWebView2WebResourceResponse *response;
};

static void event_release(struct resource_event *event)
{
    if (InterlockedDecrement(&event->ref)) return;
    if (event->request) ICoreWebView2WebResourceRequest_Release(event->request);
    if (event->response) ICoreWebView2WebResourceResponse_Release(event->response);
    free(event);
}

/* Collects the response (through its interface, whoever implemented it) and sends it to the helper. */
static void event_finish(struct resource_event *event)
{
    struct wv2_resource_response info = {event->id};
    struct wv2_resource_id end = {event->id};
    ICoreWebView2HttpResponseHeaders *headers;
    ICoreWebView2HttpHeadersCollectionIterator *it;
    WCHAR *reason = NULL, *name, *value;
    char *buf = NULL, *chunk;
    size_t size = 0, capacity = 0, reason_size;
    IStream *content = NULL;
    BOOL has;
    ULONG read;
    HRESULT hr;

    if (InterlockedExchange(&event->finished, 1)) return;

    if (!event->response)
    {
        TRACE("request %s not handled, continuing to the network\n", wine_dbgstr_longlong(event->id));
        webview2_send(event->view, WV2_CMD_RESOURCE_RESPONSE, &info, sizeof(info));
        return;
    }

    info.handled = 1;
    ICoreWebView2WebResourceResponse_get_StatusCode(event->response, (int *)&info.status);
    ICoreWebView2WebResourceResponse_get_ReasonPhrase(event->response, &reason);
    buffer_append(&buf, &size, &capacity, reason ? reason : L"");
    reason_size = size;
    CoTaskMemFree(reason);
    if (SUCCEEDED(ICoreWebView2WebResourceResponse_get_Headers(event->response, &headers)))
    {
        if (SUCCEEDED(ICoreWebView2HttpResponseHeaders_GetIterator(headers, &it)))
        {
            while (SUCCEEDED(ICoreWebView2HttpHeadersCollectionIterator_get_HasCurrentHeader(it, &has)) && has)
            {
                if (SUCCEEDED(ICoreWebView2HttpHeadersCollectionIterator_GetCurrentHeader(it, &name, &value)))
                {
                    buffer_append(&buf, &size, &capacity, name);
                    buffer_append(&buf, &size, &capacity, L": ");
                    buffer_append(&buf, &size, &capacity, value);
                    buffer_append(&buf, &size, &capacity, L"\r\n");
                    CoTaskMemFree(name);
                    CoTaskMemFree(value);
                }
                ICoreWebView2HttpHeadersCollectionIterator_MoveNext(it, &has);
            }
            ICoreWebView2HttpHeadersCollectionIterator_Release(it);
        }
        ICoreWebView2HttpResponseHeaders_Release(headers);
    }
    info.reason_size = reason_size;
    info.headers_size = size - reason_size;

    if (!(chunk = malloc(sizeof(struct wv2_resource_id) + max(CONTENT_CHUNK, sizeof(info) + size))))
    {
        free(buf);
        return;
    }
    memcpy(chunk, &info, sizeof(info));
    memcpy(chunk + sizeof(info), buf, size);
    free(buf);
    TRACE("request %s: status %u, headers %s\n", wine_dbgstr_longlong(event->id), info.status,
          debugstr_an(chunk + sizeof(info) + info.reason_size, info.headers_size));
    webview2_send(event->view, WV2_CMD_RESOURCE_RESPONSE, chunk, sizeof(info) + size);

    ICoreWebView2WebResourceResponse_get_Content(event->response, &content);
    if (content)
    {
        UINT64 total = 0;

        memcpy(chunk, &end, sizeof(end));
        for (;;)
        {
            read = 0;
            hr = IStream_Read(content, chunk + sizeof(end), CONTENT_CHUNK, &read);
            if (FAILED(hr) || !read) break;
            webview2_send(event->view, WV2_CMD_RESOURCE_DATA, chunk, sizeof(end) + read);
            total += read;
            if (hr == S_FALSE) break;
        }
        TRACE("request %s: %s content bytes, last hr %#lx\n", wine_dbgstr_longlong(event->id),
              wine_dbgstr_longlong(total), hr);
        IStream_Release(content);
    }
    free(chunk);
    webview2_send(event->view, WV2_CMD_RESOURCE_END, &end, sizeof(end));
}

/* ICoreWebView2Deferral */

static ICoreWebView2DeferralVtbl deferral_vtbl;
static struct stub_family deferral_family;

static void deferral_destroy(struct stub_object *obj)
{
    event_release(obj->data);
}

static HRESULT STDMETHODCALLTYPE deferral_Complete(ICoreWebView2Deferral *iface)
{
    struct resource_event *event = ((struct stub_object *)iface)->data;

    TRACE("iface %p.\n", iface);
    if (event->dispatching) event->deferred = FALSE;   /* completed inside the handler */
    else event_finish(event);
    return S_OK;
}

/* ICoreWebView2WebResourceRequestedEventArgs */

static ICoreWebView2WebResourceRequestedEventArgs2Vtbl args_vtbl;
static struct stub_family args_family;

static struct resource_event *event_from_args(ICoreWebView2WebResourceRequestedEventArgs2 *iface)
{
    return ((struct stub_object *)iface)->data;
}

static void args_destroy(struct stub_object *obj)
{
    event_release(obj->data);
}

static HRESULT STDMETHODCALLTYPE args_get_Request(ICoreWebView2WebResourceRequestedEventArgs2 *iface,
        ICoreWebView2WebResourceRequest **request)
{
    struct resource_event *event = event_from_args(iface);

    TRACE("iface %p, request %p.\n", iface, request);
    if (!request) return E_POINTER;
    *request = event->request;
    ICoreWebView2WebResourceRequest_AddRef(*request);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE args_get_Response(ICoreWebView2WebResourceRequestedEventArgs2 *iface,
        ICoreWebView2WebResourceResponse **response)
{
    struct resource_event *event = event_from_args(iface);

    TRACE("iface %p, response %p.\n", iface, response);
    if (!response) return E_POINTER;
    if ((*response = event->response)) ICoreWebView2WebResourceResponse_AddRef(*response);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE args_put_Response(ICoreWebView2WebResourceRequestedEventArgs2 *iface,
        ICoreWebView2WebResourceResponse *response)
{
    struct resource_event *event = event_from_args(iface);

    TRACE("iface %p, response %p.\n", iface, response);
    if (response) ICoreWebView2WebResourceResponse_AddRef(response);
    if (event->response) ICoreWebView2WebResourceResponse_Release(event->response);
    event->response = response;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE args_GetDeferral(ICoreWebView2WebResourceRequestedEventArgs2 *iface,
        ICoreWebView2Deferral **deferral)
{
    struct resource_event *event = event_from_args(iface);
    struct stub_object *obj;

    TRACE("iface %p, deferral %p.\n", iface, deferral);
    if (!deferral) return E_POINTER;
    if (!(obj = stub_create(&deferral_family))) return E_OUTOFMEMORY;
    InterlockedIncrement(&event->ref);
    obj->data = event;
    obj->destroy = deferral_destroy;
    event->deferred = TRUE;
    *deferral = (ICoreWebView2Deferral *)obj;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE args_get_ResourceContext(ICoreWebView2WebResourceRequestedEventArgs2 *iface,
        COREWEBVIEW2_WEB_RESOURCE_CONTEXT *context)
{
    TRACE("iface %p, context %p.\n", iface, context);
    if (!context) return E_POINTER;
    *context = event_from_args(iface)->context;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE args_get_RequestedSourceKind(ICoreWebView2WebResourceRequestedEventArgs2 *iface,
        COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS *kind)
{
    TRACE("iface %p, kind %p.\n", iface, kind);
    if (!kind) return E_POINTER;
    *kind = COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_DOCUMENT;
    return S_OK;
}

static HRESULT create_request(const struct wv2_resource_request *info, const char *data,
                              ICoreWebView2WebResourceRequest **out)
{
    struct stub_object *obj;
    struct request *req;
    WCHAR *headers_text;

    if (!(req = calloc(1, sizeof(*req)))) return E_OUTOFMEMORY;
    req->uri = strdupUtf8(data, info->url_size);
    req->method = strdupUtf8(data + info->url_size, info->method_size);
    req->headers = headers_create();
    headers_text = strdupUtf8(data + info->url_size + info->method_size, info->headers_size);
    if (req->headers) headers_parse(req->headers, headers_text);
    free(headers_text);
    if (info->body_size)
        req->content = SHCreateMemStream((const BYTE *)data + info->url_size + info->method_size + info->headers_size,
                                         info->body_size);
    if (!req->uri || !req->method || !req->headers || !(obj = stub_create(&request_family)))
    {
        free(req->uri);
        free(req->method);
        if (req->headers) headers_release(req->headers);
        if (req->content) IStream_Release(req->content);
        free(req);
        return E_OUTOFMEMORY;
    }
    obj->data = req;
    obj->destroy = request_destroy;
    *out = (ICoreWebView2WebResourceRequest *)obj;
    return S_OK;
}

void resource_dispatch_request(UINT32 view, ICoreWebView2 *webview, IUnknown **handlers, unsigned int count,
                               const BYTE *payload, UINT32 size)
{
    const struct wv2_resource_request *info = (const void *)payload;
    struct resource_event *event;
    struct stub_object *args;
    unsigned int i;

    if (size < sizeof(*info) || size - sizeof(*info) <
            (UINT64)info->url_size + info->method_size + info->headers_size + info->body_size)
    {
        ERR("malformed resource request\n");
        return;
    }
    if (!(event = calloc(1, sizeof(*event)))) return;
    event->ref = 1;
    event->view = view;
    event->id = info->id;
    event->context = info->context;
    if (FAILED(create_request(info, (const char *)(info + 1), &event->request)) || !(args = stub_create(&args_family)))
    {
        event_finish(event);    /* let the request continue */
        event_release(event);
        return;
    }
    InterlockedIncrement(&event->ref);
    args->data = event;
    args->destroy = args_destroy;
    TRACE("request %s: %s, context %u, %u handler(s)\n", wine_dbgstr_longlong(info->id),
          debugstr_w(((struct request *)((struct stub_object *)event->request)->data)->uri), info->context, count);

    event->dispatching = TRUE;
    for (i = 0; i < count; i++)
        ICoreWebView2WebResourceRequestedEventHandler_Invoke((ICoreWebView2WebResourceRequestedEventHandler *)handlers[i],
                webview, (ICoreWebView2WebResourceRequestedEventArgs *)args);
    event->dispatching = FALSE;

    if (!event->deferred) event_finish(event);
    ICoreWebView2WebResourceRequestedEventArgs_Release((ICoreWebView2WebResourceRequestedEventArgs *)args);
    event_release(event);
}

void resource_init(void)
{
    iterator_vtbl = *(const ICoreWebView2HttpHeadersCollectionIteratorVtbl *)stub_family_ICoreWebView2HttpHeadersCollectionIterator.vtbl;
    iterator_vtbl.GetCurrentHeader = iterator_GetCurrentHeader;
    iterator_vtbl.get_HasCurrentHeader = iterator_get_HasCurrentHeader;
    iterator_vtbl.MoveNext = iterator_MoveNext;
    iterator_family = stub_family_ICoreWebView2HttpHeadersCollectionIterator;
    iterator_family.vtbl = &iterator_vtbl;

    request_headers_vtbl = *(const ICoreWebView2HttpRequestHeadersVtbl *)stub_family_ICoreWebView2HttpRequestHeaders.vtbl;
    request_headers_vtbl.GetHeader = request_headers_GetHeader;
    request_headers_vtbl.GetHeaders = request_headers_GetHeaders;
    request_headers_vtbl.Contains = request_headers_Contains;
    request_headers_vtbl.SetHeader = request_headers_SetHeader;
    request_headers_vtbl.RemoveHeader = request_headers_RemoveHeader;
    request_headers_vtbl.GetIterator = request_headers_GetIterator;
    request_headers_family = stub_family_ICoreWebView2HttpRequestHeaders;
    request_headers_family.vtbl = &request_headers_vtbl;

    response_headers_vtbl = *(const ICoreWebView2HttpResponseHeadersVtbl *)stub_family_ICoreWebView2HttpResponseHeaders.vtbl;
    response_headers_vtbl.AppendHeader = response_headers_AppendHeader;
    response_headers_vtbl.Contains = response_headers_Contains;
    response_headers_vtbl.GetHeader = response_headers_GetHeader;
    response_headers_vtbl.GetHeaders = response_headers_GetHeaders;
    response_headers_vtbl.GetIterator = response_headers_GetIterator;
    response_headers_family = stub_family_ICoreWebView2HttpResponseHeaders;
    response_headers_family.vtbl = &response_headers_vtbl;

    request_vtbl = *(const ICoreWebView2WebResourceRequestVtbl *)stub_family_ICoreWebView2WebResourceRequest.vtbl;
    request_vtbl.get_Uri = request_get_Uri;
    request_vtbl.put_Uri = request_put_Uri;
    request_vtbl.get_Method = request_get_Method;
    request_vtbl.put_Method = request_put_Method;
    request_vtbl.get_Content = request_get_Content;
    request_vtbl.put_Content = request_put_Content;
    request_vtbl.get_Headers = request_get_Headers;
    request_family = stub_family_ICoreWebView2WebResourceRequest;
    request_family.vtbl = &request_vtbl;

    response_vtbl = *(const ICoreWebView2WebResourceResponseVtbl *)stub_family_ICoreWebView2WebResourceResponse.vtbl;
    response_vtbl.get_Content = response_get_Content;
    response_vtbl.put_Content = response_put_Content;
    response_vtbl.get_Headers = response_get_Headers;
    response_vtbl.get_StatusCode = response_get_StatusCode;
    response_vtbl.put_StatusCode = response_put_StatusCode;
    response_vtbl.get_ReasonPhrase = response_get_ReasonPhrase;
    response_vtbl.put_ReasonPhrase = response_put_ReasonPhrase;
    response_family = stub_family_ICoreWebView2WebResourceResponse;
    response_family.vtbl = &response_vtbl;

    deferral_vtbl = *(const ICoreWebView2DeferralVtbl *)stub_family_ICoreWebView2Deferral.vtbl;
    deferral_vtbl.Complete = deferral_Complete;
    deferral_family = stub_family_ICoreWebView2Deferral;
    deferral_family.vtbl = &deferral_vtbl;

    args_vtbl = *(const ICoreWebView2WebResourceRequestedEventArgs2Vtbl *)stub_family_ICoreWebView2WebResourceRequestedEventArgs2.vtbl;
    args_vtbl.get_Request = args_get_Request;
    args_vtbl.get_Response = args_get_Response;
    args_vtbl.put_Response = args_put_Response;
    args_vtbl.GetDeferral = args_GetDeferral;
    args_vtbl.get_ResourceContext = args_get_ResourceContext;
    args_vtbl.get_RequestedSourceKind = args_get_RequestedSourceKind;
    args_family = stub_family_ICoreWebView2WebResourceRequestedEventArgs2;
    args_family.vtbl = &args_vtbl;
}
