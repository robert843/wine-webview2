/*
 * ICoreWebView2Controller / ICoreWebView2 backed by the native helper process.
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"
#include "webview2.h"
#include "stubs.h"
#include "webview2_private.h"
#include "unixlib.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(webview2);

/*
 * Helper process connection.
 *
 * One helper (the native browser process) per application process, started with the first
 * environment. A reader thread blocks in the unix side and forwards every message to the UI
 * thread of the view it belongs to.
 */

static unixlib_handle_t unix_handle;
static helper_handle_t helper;
static INIT_ONCE helper_once = INIT_ONCE_STATIC_INIT;

#define UNIX_CALL(func, params) __wine_unix_call(unix_handle, unix_ ## func, params)

static CRITICAL_SECTION views_cs;
static CRITICAL_SECTION_DEBUG views_cs_debug =
{
    0, 0, &views_cs,
    { &views_cs_debug.ProcessLocksList, &views_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": views_cs") }
};
static CRITICAL_SECTION views_cs = { &views_cs_debug, -1, 0, 0, 0, 0 };
static struct list views = LIST_INIT(views);
static UINT32 next_view_id = 1;

struct view
{
    struct list entry;
    LONG ref;
    UINT32 id;
    HWND parent;
    HWND hwnd;              /* host child window */
    HWND async_hwnd;        /* async window of the UI thread that created the view */
    RECT bounds;
    BOOL visible;
    BOOL created;
    BOOL closed;
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *created_handler;
    ICoreWebView2Controller *controller;    /* weak */
    struct stub_object *webview;            /* strong, created lazily */

    /* current frame buffer announced by the helper */
    UINT32 frame_width, frame_height;
    UINT64 generation;
    /* local copy shown in the host window */
    HDC dc;
    HBITMAP dib, old_bitmap;
    void *bits;
    UINT32 dib_width, dib_height;
};

static void view_addref(struct view *view)
{
    InterlockedIncrement(&view->ref);
}

static void view_release(struct view *view)
{
    if (InterlockedDecrement(&view->ref)) return;

    TRACE("destroying view %u\n", view->id);
    EnterCriticalSection(&views_cs);
    list_remove(&view->entry);
    LeaveCriticalSection(&views_cs);
    if (view->dc)
    {
        SelectObject(view->dc, view->old_bitmap);
        DeleteObject(view->dib);
        DeleteDC(view->dc);
    }
    free(view);
}

static struct view *find_view(UINT32 id)
{
    struct view *view;

    EnterCriticalSection(&views_cs);
    LIST_FOR_EACH_ENTRY(view, &views, struct view, entry)
    {
        if (view->id == id)
        {
            view_addref(view);
            LeaveCriticalSection(&views_cs);
            return view;
        }
    }
    LeaveCriticalSection(&views_cs);
    return NULL;
}

static HRESULT helper_send(struct view *view, UINT32 type, const void *data, UINT32 size)
{
    struct helper_send_params params = {helper, type, view ? view->id : 0, data, size};

    if (!helper) return S_OK;   /* no helper: skeleton mode */
    if (UNIX_CALL(helper_send, &params))
    {
        WARN("failed to send command %u\n", type);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT helper_send_string(struct view *view, UINT32 type, const void *prefix, UINT32 prefix_size,
                                  const WCHAR *str)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    char *buf;
    HRESULT hr;

    if (len <= 0) len = 1;
    if (!(buf = malloc(prefix_size + len))) return E_OUTOFMEMORY;
    if (prefix_size) memcpy(buf, prefix, prefix_size);
    WideCharToMultiByte(CP_UTF8, 0, str, -1, buf + prefix_size, len, NULL, NULL);
    hr = helper_send(view, type, buf, prefix_size + len - 1);
    free(buf);
    return hr;
}

static void send_size(struct view *view)
{
    struct wv2_view_size size =
    {
        max(1, view->bounds.right - view->bounds.left),
        max(1, view->bounds.bottom - view->bounds.top),
        100, 0
    };
    helper_send(view, WV2_CMD_RESIZE, &size, sizeof(size));
}

static void send_visible(struct view *view)
{
    struct wv2_flag flag = {view->visible, 0};
    helper_send(view, WV2_CMD_SET_VISIBLE, &flag, sizeof(flag));
}

/* --- events from the helper, delivered on the view's UI thread --- */

struct helper_event
{
    struct wv2_msg_header header;
    BYTE payload[];
};

static void invoke_controller_created(struct view *view, HRESULT hr)
{
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *handler = view->created_handler;

    if (!handler) return;
    view->created_handler = NULL;
    TRACE("view %u: invoking controller completed handler %p, hr %#lx\n", view->id, handler, hr);
    hr = ICoreWebView2CreateCoreWebView2ControllerCompletedHandler_Invoke(handler, hr,
            SUCCEEDED(hr) ? view->controller : NULL);
    TRACE("handler returned %#lx\n", hr);
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandler_Release(handler);
    ICoreWebView2Controller_Release(view->controller);  /* reference held while pending */
}

static BOOL ensure_dib(struct view *view, UINT32 width, UINT32 height)
{
    BITMAPINFO info = {{sizeof(BITMAPINFOHEADER)}};
    HBITMAP dib;
    void *bits;

    if (view->dib && view->dib_width == width && view->dib_height == height) return TRUE;

    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -(LONG)height;    /* top-down, like the helper's frames */
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    if (!(dib = CreateDIBSection(NULL, &info, DIB_RGB_COLORS, &bits, NULL, 0)))
    {
        ERR("CreateDIBSection %ux%u failed\n", width, height);
        return FALSE;
    }
    if (!view->dc)
    {
        view->dc = CreateCompatibleDC(NULL);
        view->old_bitmap = SelectObject(view->dc, dib);
    }
    else
    {
        SelectObject(view->dc, dib);
        DeleteObject(view->dib);
    }
    view->dib = dib;
    view->bits = bits;
    view->dib_width = width;
    view->dib_height = height;
    return TRUE;
}

static void handle_frame_ready(struct view *view, const struct wv2_frame_ready *ready)
{
    struct helper_copy_frame_params params = {helper, view->id, ready->slot, ready->generation};

    if (ready->generation == view->generation && ensure_dib(view, view->frame_width, view->frame_height))
    {
        params.dst = view->bits;
        params.dst_stride = view->dib_width * 4;
        params.width = view->dib_width;
        params.height = view->dib_height;
    }
    /* Always called: it also hands the slot back to the helper. */
    if (UNIX_CALL(helper_copy_frame, &params) || !params.dst) return;
    if (view->hwnd)
    {
        RECT rect = {ready->dirty_left, ready->dirty_top, ready->dirty_right, ready->dirty_bottom};
        InvalidateRect(view->hwnd, &rect, FALSE);
    }
}

static void dispatch_event_cb(void *arg)
{
    struct helper_event *event = arg;
    UINT32 payload = event->header.size - sizeof(event->header);
    struct view *view = find_view(event->header.view);

    if (!view)
    {
        free(event);
        return;
    }

    switch (event->header.type)
    {
    case WV2_EVT_VIEW_CREATED:
        TRACE("view %u created\n", view->id);
        view->created = TRUE;
        send_size(view);
        send_visible(view);
        invoke_controller_created(view, S_OK);
        break;
    case WV2_EVT_FRAME_BUFFER:
        if (payload >= sizeof(struct wv2_frame_buffer))
        {
            const struct wv2_frame_buffer *fb = (const void *)event->payload;
            view->frame_width = fb->width;
            view->frame_height = fb->height;
            view->generation = fb->generation;
        }
        break;
    case WV2_EVT_FRAME_READY:
        if (payload >= sizeof(struct wv2_frame_ready))
            handle_frame_ready(view, (const void *)event->payload);
        break;
    case WV2_EVT_LOAD_STATE:
        if (payload >= sizeof(struct wv2_flag))
            TRACE("view %u: %s\n", view->id, ((const struct wv2_flag *)event->payload)->value ? "loading" : "loaded");
        break;
    case WV2_EVT_ERROR:
        ERR("view %u: helper error %s\n", view->id, debugstr_an((const char *)event->payload, payload));
        invoke_controller_created(view, E_FAIL);
        break;
    default:
        FIXME("view %u: unhandled event %#x\n", view->id, event->header.type);
        break;
    }
    view_release(view);
    free(event);
}

static DWORD WINAPI reader_thread(void *arg)
{
    struct helper_recv_params params = {helper};
    struct helper_event *event;
    struct view *view;
    UINT32 payload;

    if (!(params.buffer = malloc(WV2_MAX_MESSAGE))) return 1;
    params.size = WV2_MAX_MESSAGE;

    while (!UNIX_CALL(helper_recv, &params))
    {
        payload = min(params.header.size - sizeof(params.header), params.size);
        if (!(view = find_view(params.header.view)))
        {
            TRACE("event %#x for unknown view %u\n", params.header.type, params.header.view);
            /* Return the slot so the helper does not stall on a view we no longer track. */
            if (params.header.type == WV2_EVT_FRAME_READY)
            {
                struct helper_copy_frame_params copy = {helper, params.header.view,
                        ((struct wv2_frame_ready *)params.buffer)->slot,
                        ((struct wv2_frame_ready *)params.buffer)->generation};
                UNIX_CALL(helper_copy_frame, &copy);
            }
            continue;
        }
        if ((event = malloc(sizeof(*event) + payload)))
        {
            event->header = params.header;
            event->header.size = sizeof(event->header) + payload;
            memcpy(event->payload, params.buffer, payload);
            if (!webview2_queue_async_to(view->async_hwnd, dispatch_event_cb, event)) free(event);
        }
        view_release(view);
    }
    ERR("helper connection lost\n");
    free(params.buffer);
    return 0;
}

static NTSTATUS load_unixlib(void)
{
    WCHAR path[MAX_PATH], *p;
    UNICODE_STRING nt_name;
    unixlib_module_t module;
    NTSTATUS status;

    /* The unix library lives next to the DLL (overridable for development). */
    if (!GetEnvironmentVariableW(L"WINE_WEBVIEW2_UNIXLIB", path, ARRAY_SIZE(path)))
    {
        GetModuleFileNameW(webview2_instance, path, ARRAY_SIZE(path));
        if ((p = wcsrchr(path, '\\'))) p[1] = 0;
        wcscat(path, L"embeddedbrowserwebview.so");
    }
    if (!RtlDosPathNameToNtPathName_U(path, &nt_name, NULL, NULL)) return STATUS_OBJECT_PATH_INVALID;
    status = __wine_load_unix_lib(&nt_name, &module, &unix_handle);
    RtlFreeUnicodeString(&nt_name);
    if (status) ERR("cannot load unix library %s: %#lx (Wine >= 11.2 required)\n", debugstr_w(path), status);
    else TRACE("loaded unix library %s\n", debugstr_w(path));
    return status;
}

static BOOL WINAPI start_helper_once(INIT_ONCE *once, void *param, void **context)
{
    struct helper_start_params params = {0};
    HANDLE thread;

    if (load_unixlib()) return TRUE;
    if (UNIX_CALL(helper_start, &params))
    {
        ERR("cannot start the helper process, continuing without rendering\n");
        return TRUE;
    }
    helper = params.handle;
    TRACE("helper pid %u, protocol %u\n", params.helper_pid, params.helper_version);
    if ((thread = CreateThread(NULL, 0, reader_thread, NULL, 0, NULL))) CloseHandle(thread);
    return TRUE;
}

void helper_ensure_started(void)
{
    InitOnceExecuteOnce(&helper_once, start_helper_once, NULL, NULL);
}

/*
 * Host window: child of the application window showing the latest frame.
 */

static const WCHAR host_class_name[] = L"WineWebView2Host";

static LRESULT CALLBACK host_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    struct view *view = (struct view *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    PAINTSTRUCT ps;
    HDC hdc;

    switch (msg)
    {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        if (view && view->dc)
        {
            BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left,
                   ps.rcPaint.bottom - ps.rcPaint.top, view->dc, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
        }
        else FillRect(hdc, &ps.rcPaint, GetStockObject(WHITE_BRUSH));
        EndPaint(hwnd, &ps);
        return 0;
    case WM_NCDESTROY:
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        break;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

/*
 * ICoreWebView2Controller and ICoreWebView2 overrides.
 */

static ICoreWebView2Controller4Vtbl controller_vtbl;
static struct stub_family controller_family;
static ICoreWebView2_29Vtbl webview_vtbl;
static struct stub_family webview_family;

static struct view *view_from_controller(ICoreWebView2Controller4 *iface)
{
    return ((struct stub_object *)iface)->data;
}

static struct view *view_from_webview(ICoreWebView2_29 *iface)
{
    return ((struct stub_object *)iface)->data;
}

static void close_view(struct view *view)
{
    if (view->closed) return;
    view->closed = TRUE;
    helper_send(view, WV2_CMD_CLOSE_VIEW, NULL, 0);
    if (view->hwnd)
    {
        DestroyWindow(view->hwnd);
        view->hwnd = NULL;
    }
    if (view->webview)
    {
        view->webview->data = NULL;
        ICoreWebView2_Release((ICoreWebView2 *)view->webview);
        view->webview = NULL;
    }
}

static void controller_destroy(struct stub_object *obj)
{
    struct view *view = obj->data;

    if (!view) return;
    close_view(view);
    view_release(view);
}

static HRESULT STDMETHODCALLTYPE controller_get_CoreWebView2(ICoreWebView2Controller4 *iface, ICoreWebView2 **webview)
{
    struct view *view = view_from_controller(iface);

    TRACE("iface %p, webview %p.\n", iface, webview);
    if (!webview) return E_POINTER;
    if (view->closed) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    if (!view->webview)
    {
        if (!(view->webview = stub_create(&webview_family))) return E_OUTOFMEMORY;
        view->webview->data = view;     /* weak; cleared in close_view() */
    }
    *webview = (ICoreWebView2 *)view->webview;
    ICoreWebView2_AddRef(*webview);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE controller_get_Bounds(ICoreWebView2Controller4 *iface, RECT *bounds)
{
    struct view *view = view_from_controller(iface);

    TRACE("iface %p, bounds %p.\n", iface, bounds);
    if (!bounds) return E_POINTER;
    *bounds = view->bounds;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE controller_put_Bounds(ICoreWebView2Controller4 *iface, RECT bounds)
{
    struct view *view = view_from_controller(iface);

    TRACE("iface %p, bounds %s.\n", iface, wine_dbgstr_rect(&bounds));
    if (view->closed) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    view->bounds = bounds;
    if (view->hwnd)
        SetWindowPos(view->hwnd, NULL, bounds.left, bounds.top, bounds.right - bounds.left,
                     bounds.bottom - bounds.top, SWP_NOZORDER | SWP_NOACTIVATE);
    if (view->created) send_size(view);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE controller_get_IsVisible(ICoreWebView2Controller4 *iface, BOOL *visible)
{
    struct view *view = view_from_controller(iface);

    TRACE("iface %p, visible %p.\n", iface, visible);
    if (!visible) return E_POINTER;
    *visible = view->visible;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE controller_put_IsVisible(ICoreWebView2Controller4 *iface, BOOL visible)
{
    struct view *view = view_from_controller(iface);

    TRACE("iface %p, visible %d.\n", iface, visible);
    if (view->closed) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    view->visible = !!visible;
    if (view->hwnd) ShowWindow(view->hwnd, visible ? SW_SHOWNA : SW_HIDE);
    if (view->created) send_visible(view);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE controller_get_ParentWindow(ICoreWebView2Controller4 *iface, HWND *parent)
{
    struct view *view = view_from_controller(iface);

    TRACE("iface %p, parent %p.\n", iface, parent);
    if (!parent) return E_POINTER;
    *parent = view->parent;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE controller_put_ParentWindow(ICoreWebView2Controller4 *iface, HWND parent)
{
    struct view *view = view_from_controller(iface);

    TRACE("iface %p, parent %p.\n", iface, parent);
    if (view->closed) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    view->parent = parent;
    if (view->hwnd) SetParent(view->hwnd, parent);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE controller_Close(ICoreWebView2Controller4 *iface)
{
    struct view *view = view_from_controller(iface);

    TRACE("iface %p.\n", iface);
    close_view(view);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE webview_Navigate(ICoreWebView2_29 *iface, LPCWSTR uri)
{
    struct view *view = view_from_webview(iface);

    TRACE("iface %p, uri %s.\n", iface, debugstr_w(uri));
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    if (!uri) return E_INVALIDARG;
    return helper_send_string(view, WV2_CMD_NAVIGATE, NULL, 0, uri);
}

static HRESULT STDMETHODCALLTYPE webview_NavigateToString(ICoreWebView2_29 *iface, LPCWSTR html)
{
    struct view *view = view_from_webview(iface);

    TRACE("iface %p, html %s.\n", iface, debugstr_w(html));
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    if (!html) return E_INVALIDARG;
    return helper_send_string(view, WV2_CMD_NAVIGATE_TO_STRING, NULL, 0, html);
}

struct script_result_ctx
{
    ICoreWebView2ExecuteScriptCompletedHandler *handler;
};

static void script_result_cb(void *arg)
{
    struct script_result_ctx *ctx = arg;

    /* E3: real results come from the renderer; report "null" like a script without a value. */
    ICoreWebView2ExecuteScriptCompletedHandler_Invoke(ctx->handler, S_OK, L"null");
    ICoreWebView2ExecuteScriptCompletedHandler_Release(ctx->handler);
    free(ctx);
}

static HRESULT STDMETHODCALLTYPE webview_ExecuteScript(ICoreWebView2_29 *iface, LPCWSTR script,
        ICoreWebView2ExecuteScriptCompletedHandler *handler)
{
    static LONG64 next_id;
    struct view *view = view_from_webview(iface);
    struct wv2_script info = {InterlockedIncrement64(&next_id)};
    struct script_result_ctx *ctx;
    HRESULT hr;

    TRACE("iface %p, script %s, handler %p.\n", iface, debugstr_w(script), handler);
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    if (!script) return E_INVALIDARG;
    if (FAILED(hr = helper_send_string(view, WV2_CMD_EXECUTE_SCRIPT, &info, sizeof(info), script))) return hr;
    if (handler && (ctx = malloc(sizeof(*ctx))))
    {
        ctx->handler = handler;
        ICoreWebView2ExecuteScriptCompletedHandler_AddRef(handler);
        if (!webview2_queue_async(script_result_cb, ctx))
        {
            ICoreWebView2ExecuteScriptCompletedHandler_Release(handler);
            free(ctx);
        }
    }
    return S_OK;
}

struct add_script_ctx
{
    ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler *handler;
    WCHAR id[24];
};

static void add_script_cb(void *arg)
{
    struct add_script_ctx *ctx = arg;

    ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler_Invoke(ctx->handler, S_OK, ctx->id);
    ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler_Release(ctx->handler);
    free(ctx);
}

static HRESULT STDMETHODCALLTYPE webview_AddScriptToExecuteOnDocumentCreated(ICoreWebView2_29 *iface,
        LPCWSTR script, ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler *handler)
{
    static LONG64 next_id;
    struct view *view = view_from_webview(iface);
    struct wv2_script info = {InterlockedIncrement64(&next_id)};
    struct add_script_ctx *ctx;
    HRESULT hr;

    TRACE("iface %p, script %s, handler %p.\n", iface, debugstr_w(script), handler);
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    if (!script) return E_INVALIDARG;
    if (FAILED(hr = helper_send_string(view, WV2_CMD_ADD_DOCUMENT_SCRIPT, &info, sizeof(info), script))) return hr;
    if (handler && (ctx = malloc(sizeof(*ctx))))
    {
        ctx->handler = handler;
        swprintf(ctx->id, ARRAY_SIZE(ctx->id), L"%I64u", info.id);
        ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler_AddRef(handler);
        if (!webview2_queue_async(add_script_cb, ctx))
        {
            ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler_Release(handler);
            free(ctx);
        }
    }
    return S_OK;
}

/*
 * Controller creation.
 */

static void controller_created_skeleton_cb(void *arg)
{
    struct view *view = arg;

    invoke_controller_created(view, S_OK);
    view_release(view);
}

HRESULT controller_create(HWND parent, ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *handler)
{
    struct wv2_view_size size = {1, 1, 100, 0};
    struct stub_object *controller;
    struct view *view;

    if (!handler) return E_POINTER;
    if (!IsWindow(parent)) return E_INVALIDARG;

    if (!(view = calloc(1, sizeof(*view)))) return E_OUTOFMEMORY;
    if (!(controller = stub_create(&controller_family)))
    {
        free(view);
        return E_OUTOFMEMORY;
    }
    view->ref = 1;      /* owned by the controller */
    view->parent = parent;
    view->async_hwnd = webview2_get_async_window();
    view->controller = (ICoreWebView2Controller *)controller;
    view->visible = TRUE;
    GetClientRect(parent, &view->bounds);
    controller->data = view;
    controller->destroy = controller_destroy;

    EnterCriticalSection(&views_cs);
    view->id = next_view_id++;
    list_add_tail(&views, &view->entry);
    LeaveCriticalSection(&views_cs);

    view->hwnd = CreateWindowExW(0, host_class_name, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                                 view->bounds.left, view->bounds.top,
                                 view->bounds.right - view->bounds.left, view->bounds.bottom - view->bounds.top,
                                 parent, NULL, webview2_instance, NULL);
    if (view->hwnd) SetWindowLongPtrW(view->hwnd, GWLP_USERDATA, (LONG_PTR)view);
    else ERR("cannot create host window, error %lu\n", GetLastError());

    view->created_handler = handler;
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandler_AddRef(handler);
    ICoreWebView2Controller_AddRef(view->controller);  /* released once the handler ran */

    TRACE("view %u, parent %p, bounds %s, host window %p\n", view->id, parent,
          wine_dbgstr_rect(&view->bounds), view->hwnd);

    if (helper)
    {
        size.width = max(1, view->bounds.right - view->bounds.left);
        size.height = max(1, view->bounds.bottom - view->bounds.top);
        if (SUCCEEDED(helper_send(view, WV2_CMD_CREATE_VIEW, &size, sizeof(size))))
        {
            ICoreWebView2Controller_Release(view->controller);  /* creation reference */
            return S_OK;    /* handler runs on WV2_EVT_VIEW_CREATED */
        }
    }

    /* Skeleton mode (no helper): complete right away, asynchronously. */
    view_addref(view);
    webview2_queue_async(controller_created_skeleton_cb, view);
    ICoreWebView2Controller_Release(view->controller);  /* creation reference */
    return S_OK;
}

void controller_init(void)
{
    WNDCLASSW cls = {0};

    cls.lpfnWndProc = host_wndproc;
    cls.hInstance = webview2_instance;
    cls.hCursor = LoadCursorW(NULL, (const WCHAR *)IDC_ARROW);
    cls.lpszClassName = host_class_name;
    RegisterClassW(&cls);

    controller_vtbl = *(const ICoreWebView2Controller4Vtbl *)stub_family_ICoreWebView2Controller4.vtbl;
    controller_vtbl.get_CoreWebView2 = controller_get_CoreWebView2;
    controller_vtbl.get_Bounds = controller_get_Bounds;
    controller_vtbl.put_Bounds = controller_put_Bounds;
    controller_vtbl.get_IsVisible = controller_get_IsVisible;
    controller_vtbl.put_IsVisible = controller_put_IsVisible;
    controller_vtbl.get_ParentWindow = controller_get_ParentWindow;
    controller_vtbl.put_ParentWindow = controller_put_ParentWindow;
    controller_vtbl.Close = controller_Close;
    controller_family = stub_family_ICoreWebView2Controller4;
    controller_family.vtbl = &controller_vtbl;

    webview_vtbl = *(const ICoreWebView2_29Vtbl *)stub_family_ICoreWebView2_29.vtbl;
    webview_vtbl.Navigate = webview_Navigate;
    webview_vtbl.NavigateToString = webview_NavigateToString;
    webview_vtbl.ExecuteScript = webview_ExecuteScript;
    webview_vtbl.AddScriptToExecuteOnDocumentCreated = webview_AddScriptToExecuteOnDocumentCreated;
    webview_family = stub_family_ICoreWebView2_29;
    webview_family.vtbl = &webview_vtbl;
}
