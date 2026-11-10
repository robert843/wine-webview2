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

/*
 * Event handler registrations (add_X / remove_X).
 */

struct handler_entry
{
    INT64 token;
    IUnknown *handler;
};

struct handler_list
{
    struct handler_entry *entries;
    unsigned int count, capacity;
};

static HRESULT handler_list_add(struct handler_list *list, IUnknown *handler, EventRegistrationToken *token)
{
    static LONG64 next_token;
    struct handler_entry *entries;

    if (!handler || !token) return E_POINTER;
    if (list->count == list->capacity)
    {
        unsigned int capacity = max(4, list->capacity * 2);
        if (!(entries = realloc(list->entries, capacity * sizeof(*entries)))) return E_OUTOFMEMORY;
        list->entries = entries;
        list->capacity = capacity;
    }
    IUnknown_AddRef(handler);
    list->entries[list->count].handler = handler;
    list->entries[list->count].token = token->value = InterlockedIncrement64(&next_token);
    list->count++;
    return S_OK;
}

static HRESULT handler_list_remove(struct handler_list *list, EventRegistrationToken token)
{
    unsigned int i;

    for (i = 0; i < list->count; i++)
    {
        if (list->entries[i].token != token.value) continue;
        IUnknown_Release(list->entries[i].handler);
        memmove(&list->entries[i], &list->entries[i + 1], (list->count - i - 1) * sizeof(*list->entries));
        list->count--;
        break;
    }
    return S_OK;    /* WebView2 ignores unknown tokens */
}

/* Snapshot with references, so handlers may (un)register while being invoked. */
static IUnknown **handler_list_snapshot(struct handler_list *list, unsigned int *count)
{
    IUnknown **ret;
    unsigned int i;

    *count = 0;
    if (!list->count || !(ret = malloc(list->count * sizeof(*ret)))) return NULL;
    for (i = 0; i < list->count; i++)
    {
        ret[i] = list->entries[i].handler;
        IUnknown_AddRef(ret[i]);
    }
    *count = list->count;
    return ret;
}

static void handler_list_clear(struct handler_list *list)
{
    unsigned int i;

    for (i = 0; i < list->count; i++) IUnknown_Release(list->entries[i].handler);
    free(list->entries);
    memset(list, 0, sizeof(*list));
}

static WCHAR *strdupAtoW_n(const char *str, int len)
{
    int size = MultiByteToWideChar(CP_UTF8, 0, str, len, NULL, 0);
    WCHAR *ret;

    if (!(ret = malloc((size + 1) * sizeof(WCHAR)))) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, str, len, ret, size);
    ret[size] = 0;
    return ret;
}

static WCHAR *co_strdupW(const WCHAR *str)
{
    size_t size = (wcslen(str) + 1) * sizeof(WCHAR);
    WCHAR *ret = CoTaskMemAlloc(size);
    if (ret) memcpy(ret, str, size);
    return ret;
}

/* Encodes str as a JSON string literal. */
static WCHAR *json_quote(const WCHAR *str)
{
    size_t len = wcslen(str), pos = 0;
    WCHAR *ret = malloc((len * 6 + 3) * sizeof(WCHAR));

    if (!ret) return NULL;
    ret[pos++] = '"';
    for (; *str; str++)
    {
        switch (*str)
        {
        case '"':  ret[pos++] = '\\'; ret[pos++] = '"'; break;
        case '\\': ret[pos++] = '\\'; ret[pos++] = '\\'; break;
        case '\n': ret[pos++] = '\\'; ret[pos++] = 'n'; break;
        case '\r': ret[pos++] = '\\'; ret[pos++] = 'r'; break;
        case '\t': ret[pos++] = '\\'; ret[pos++] = 't'; break;
        default:
            if (*str < 0x20 || *str == 0x2028 || *str == 0x2029)
                pos += swprintf(ret + pos, 7, L"\\u%04x", *str);
            else
                ret[pos++] = *str;
        }
    }
    ret[pos++] = '"';
    ret[pos] = 0;
    return ret;
}

static int hex_digit(WCHAR c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Decodes a JSON string literal (the whole input); returns NULL if json is not a string. */
static WCHAR *json_unquote(const WCHAR *json)
{
    size_t len;
    WCHAR *ret, *out;
    int i, d;

    while (*json == ' ' || *json == '\t' || *json == '\n' || *json == '\r') json++;
    if (*json != '"') return NULL;
    len = wcslen(json);
    if (!(out = ret = CoTaskMemAlloc(len * sizeof(WCHAR)))) return NULL;
    for (json++; *json && *json != '"'; json++)
    {
        if (*json != '\\') { *out++ = *json; continue; }
        switch (*++json)
        {
        case 'b': *out++ = '\b'; break;
        case 'f': *out++ = '\f'; break;
        case 'n': *out++ = '\n'; break;
        case 'r': *out++ = '\r'; break;
        case 't': *out++ = '\t'; break;
        case 'u':
            *out = 0;
            for (i = 0; i < 4; i++)
            {
                if ((d = hex_digit(json[1])) < 0) goto fail;
                *out = *out * 16 + d;
                json++;
            }
            out++;
            break;
        case 0: goto fail;
        default: *out++ = *json; break;
        }
    }
    if (*json != '"') goto fail;
    *out = 0;
    return ret;
fail:
    CoTaskMemFree(ret);
    return NULL;
}

enum view_event
{
    EVENT_NAVIGATION_STARTING,
    EVENT_CONTENT_LOADING,
    EVENT_SOURCE_CHANGED,
    EVENT_HISTORY_CHANGED,
    EVENT_DOM_CONTENT_LOADED,
    EVENT_NAVIGATION_COMPLETED,
    EVENT_DOCUMENT_TITLE_CHANGED,
    EVENT_COUNT
};

struct pending_script
{
    struct list entry;
    UINT64 id;
    ICoreWebView2ExecuteScriptCompletedHandler *handler;
};

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
    struct handler_list web_message_handlers;
    struct handler_list web_resource_handlers;
    struct handler_list events[EVENT_COUNT];
    WCHAR *source;
    WCHAR *title;
    BOOL can_go_back, can_go_forward;
    struct list pending_scripts;            /* struct pending_script */
    HCURSOR cursor;
    BOOL tracking_mouse;

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
    unsigned int i;

    if (InterlockedDecrement(&view->ref)) return;

    TRACE("destroying view %u\n", view->id);
    handler_list_clear(&view->web_message_handlers);
    handler_list_clear(&view->web_resource_handlers);
    for (i = 0; i < EVENT_COUNT; i++) handler_list_clear(&view->events[i]);
    free(view->source);
    free(view->title);
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

HRESULT webview2_send(UINT32 view, UINT32 type, const void *data, UINT32 size)
{
    struct helper_send_params params = {helper, type, view, data, size};

    if (!helper) return E_FAIL;
    return UNIX_CALL(helper_send, &params) ? E_FAIL : S_OK;
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

static void handle_web_message(struct view *view, const BYTE *payload, UINT32 size);
static void handle_script_result(struct view *view, const BYTE *payload, UINT32 size);

/* All ICoreWebView2*EventHandler share this layout: Invoke(sender, args). */
typedef struct
{
    const struct
    {
        HRESULT (STDMETHODCALLTYPE *QueryInterface)(void *, REFIID, void **);
        ULONG (STDMETHODCALLTYPE *AddRef)(void *);
        ULONG (STDMETHODCALLTYPE *Release)(void *);
        HRESULT (STDMETHODCALLTYPE *Invoke)(void *, void *sender, void *args);
    } *lpVtbl;
} generic_event_handler;

static void raise_event(struct view *view, enum view_event event, IUnknown *args)
{
    unsigned int i, count;
    IUnknown **handlers;

    if (!view->webview) return;
    handlers = handler_list_snapshot(&view->events[event], &count);
    for (i = 0; i < count; i++)
    {
        generic_event_handler *handler = (generic_event_handler *)handlers[i];
        handler->lpVtbl->Invoke(handler, view->webview, args);
        IUnknown_Release(handlers[i]);
    }
    free(handlers);
}

static void raise_navigation_event(struct view *view, enum view_event event, enum navigation_args_kind kind,
                                   const struct navigation_info *info, const WCHAR *uri)
{
    IUnknown *args = navigation_create_args(kind, info, uri);

    if (!args) return;
    raise_event(view, event, args);
    if (event == EVENT_NAVIGATION_STARTING && navigation_args_cancelled(args))
    {
        FIXME("view %u: navigation %s cancelled after it started, stopping the load\n", view->id,
              wine_dbgstr_longlong(info->id));
        helper_send(view, WV2_CMD_STOP, NULL, 0);
    }
    IUnknown_Release(args);
}

static void handle_navigation_event(struct view *view, UINT32 type, const BYTE *payload, UINT32 size)
{
    const struct wv2_navigation *nav = (const void *)payload;
    const struct wv2_navigation_completed *done = (const void *)payload;
    struct navigation_info info = {0};
    WCHAR *str;

    switch (type)
    {
    case WV2_EVT_NAVIGATION_STARTING:
        if (size < sizeof(*nav)) return;
        info.id = nav->id;
        info.user_initiated = nav->a;
        info.redirected = nav->b;
        str = strdupAtoW_n((const char *)(nav + 1), size - sizeof(*nav));
        TRACE("view %u: NavigationStarting %s %s\n", view->id, wine_dbgstr_longlong(info.id), debugstr_w(str));
        raise_navigation_event(view, EVENT_NAVIGATION_STARTING, NAVIGATION_ARGS_STARTING, &info, str);
        free(str);
        break;
    case WV2_EVT_SOURCE_CHANGED:
        if (!(str = strdupAtoW_n((const char *)payload, size))) return;
        info.new_document = !view->source || wcscmp(view->source, str);
        free(view->source);
        view->source = str;
        TRACE("view %u: SourceChanged %s\n", view->id, debugstr_w(str));
        raise_navigation_event(view, EVENT_SOURCE_CHANGED, NAVIGATION_ARGS_SOURCE_CHANGED, &info, NULL);
        break;
    case WV2_EVT_CONTENT_LOADING:
        if (size < sizeof(*nav)) return;
        info.id = nav->id;
        info.error_page = nav->a;
        raise_navigation_event(view, EVENT_CONTENT_LOADING, NAVIGATION_ARGS_CONTENT_LOADING, &info, NULL);
        break;
    case WV2_EVT_HISTORY_CHANGED:
        if (size < sizeof(*nav)) return;
        view->can_go_back = nav->a;
        view->can_go_forward = nav->b;
        raise_event(view, EVENT_HISTORY_CHANGED, NULL);
        break;
    case WV2_EVT_NAVIGATION_COMPLETED:
        if (size < sizeof(*done)) return;
        info.id = done->id;
        info.success = done->success;
        info.http_status = done->http_status;
        info.web_error = done->success ? COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN : navigation_web_error(done->net_error);
        TRACE("view %u: NavigationCompleted %s success %d, status %d, net error %d\n", view->id,
              wine_dbgstr_longlong(info.id), info.success, info.http_status, done->net_error);
        if (done->success)
            raise_navigation_event(view, EVENT_DOM_CONTENT_LOADED, NAVIGATION_ARGS_DOM_CONTENT_LOADED, &info, NULL);
        raise_navigation_event(view, EVENT_NAVIGATION_COMPLETED, NAVIGATION_ARGS_COMPLETED, &info, NULL);
        break;
    case WV2_EVT_TITLE_CHANGED:
        if (!(str = strdupAtoW_n((const char *)payload, size))) return;
        free(view->title);
        view->title = str;
        raise_event(view, EVENT_DOCUMENT_TITLE_CHANGED, NULL);
        break;
    }
}

/* cef_cursor_type_t -> system cursor */
static void set_view_cursor(struct view *view, UINT32 type)
{
    static const WORD cursors[] =
    {
        32512 /* IDC_ARROW */, 32515 /* IDC_CROSS */, 32649 /* IDC_HAND */, 32513 /* IDC_IBEAM */,
        32514 /* IDC_WAIT */, 32651 /* IDC_HELP */,
        32644, 32645, 32643, 32642, 32645, 32642, 32643, 32644,     /* E N NE NW S SE SW W resize */
        32645, 32644, 32643, 32642, 32644, 32645,                   /* NS EW NESW NWSE column row */
        32646, 32646, 32646, 32646, 32646, 32646, 32646, 32646, 32646, /* panning */
        32646 /* move */, 32513 /* vertical text */, 32515 /* cell */, 32512 /* context menu */,
        32512 /* alias */, 32650 /* progress (IDC_APPSTARTING) */, 32648 /* no drop */,
        32512 /* copy */, 0 /* none */, 32648 /* not allowed */,
    };
    POINT pt;
    WORD id = type < ARRAY_SIZE(cursors) ? cursors[type] : 32512;

    view->cursor = id ? LoadCursorW(NULL, MAKEINTRESOURCEW(id)) : NULL;
    if (view->hwnd && GetCursorPos(&pt) && WindowFromPoint(pt) == view->hwnd) SetCursor(view->cursor);
}

static UINT32 get_modifiers(WPARAM mouse_keys, BOOL mouse)
{
    UINT32 flags = 0;

    if (GetKeyState(VK_SHIFT) & 0x8000) flags |= WV2_FLAG_SHIFT_DOWN;
    if (GetKeyState(VK_CONTROL) & 0x8000) flags |= WV2_FLAG_CONTROL_DOWN;
    if (GetKeyState(VK_MENU) & 0x8000) flags |= WV2_FLAG_ALT_DOWN;
    if (GetKeyState(VK_CAPITAL) & 1) flags |= WV2_FLAG_CAPS_LOCK_ON;
    if (GetKeyState(VK_NUMLOCK) & 1) flags |= WV2_FLAG_NUM_LOCK_ON;
    if (mouse)
    {
        if (mouse_keys & MK_LBUTTON) flags |= WV2_FLAG_LEFT_BUTTON;
        if (mouse_keys & MK_MBUTTON) flags |= WV2_FLAG_MIDDLE_BUTTON;
        if (mouse_keys & MK_RBUTTON) flags |= WV2_FLAG_RIGHT_BUTTON;
    }
    return flags;
}

static void send_mouse(struct view *view, UINT32 kind, int x, int y, UINT32 button, UINT32 clicks,
                       WPARAM keys, int dx, int dy)
{
    struct wv2_mouse mouse = {kind, x, y, button, clicks, get_modifiers(keys, TRUE), dx, dy};
    helper_send(view, WV2_CMD_MOUSE, &mouse, sizeof(mouse));
}

/* Windows scan code (set 1) -> X11 keycode (evdev + 8); CEF derives KeyboardEvent.code from it. */
static int scancode_to_x11(LPARAM lparam)
{
    static const struct { BYTE scan, evdev; } extended[] =
    {
        {0x1c, 96 /* KP enter */}, {0x1d, 97 /* right ctrl */}, {0x35, 98 /* KP divide */},
        {0x37, 99 /* sysrq */}, {0x38, 100 /* right alt */}, {0x47, 102 /* home */}, {0x48, 103 /* up */},
        {0x49, 104 /* page up */}, {0x4b, 105 /* left */}, {0x4d, 106 /* right */}, {0x4f, 107 /* end */},
        {0x50, 108 /* down */}, {0x51, 109 /* page down */}, {0x52, 110 /* insert */},
        {0x53, 111 /* delete */}, {0x5b, 125 /* left meta */}, {0x5c, 126 /* right meta */},
        {0x5d, 127 /* menu */},
    };
    BYTE scan = (lparam >> 16) & 0xff;
    unsigned int i;

    if (!(lparam & (1 << 24))) return scan ? scan + 8 : 0;
    for (i = 0; i < ARRAY_SIZE(extended); i++)
        if (extended[i].scan == scan) return extended[i].evdev + 8;
    return 0;
}

static void send_key(struct view *view, UINT msg, WPARAM wparam, LPARAM lparam)
{
    struct wv2_key key = {0};

    key.modifiers = get_modifiers(0, FALSE);
    key.windows_key_code = wparam;
    key.native_key_code = scancode_to_x11(lparam);
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        key.kind = 0;   /* KEYEVENT_RAWKEYDOWN */
        if (lparam & (1 << 30)) key.modifiers |= WV2_FLAG_IS_REPEAT;
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        key.kind = 2;   /* KEYEVENT_KEYUP */
        break;
    default:            /* WM_CHAR, WM_SYSCHAR */
        key.kind = 3;   /* KEYEVENT_CHAR */
        key.character = key.unmodified_character = wparam;
        break;
    }
    if (msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP || msg == WM_SYSCHAR) key.is_system_key = 1;
    if (wparam >= VK_NUMPAD0 && wparam <= VK_DIVIDE) key.modifiers |= WV2_FLAG_IS_KEY_PAD;
    helper_send(view, WV2_CMD_KEY, &key, sizeof(key));
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
    case WV2_EVT_WEB_MESSAGE:
        handle_web_message(view, event->payload, payload);
        break;
    case WV2_EVT_NAVIGATION_STARTING:
    case WV2_EVT_SOURCE_CHANGED:
    case WV2_EVT_CONTENT_LOADING:
    case WV2_EVT_HISTORY_CHANGED:
    case WV2_EVT_NAVIGATION_COMPLETED:
    case WV2_EVT_TITLE_CHANGED:
        handle_navigation_event(view, event->header.type, event->payload, payload);
        break;
    case WV2_EVT_CURSOR:
        if (payload >= sizeof(struct wv2_flag))
            set_view_cursor(view, ((const struct wv2_flag *)event->payload)->value);
        break;
    case WV2_EVT_SCRIPT_RESULT:
        handle_script_result(view, event->payload, payload);
        break;
    case WV2_EVT_RESOURCE_REQUEST:
    {
        unsigned int count;
        IUnknown **handlers = view->webview ? handler_list_snapshot(&view->web_resource_handlers, &count) : NULL;

        if (!handlers) count = 0;
        resource_dispatch_request(view->id, (ICoreWebView2 *)view->webview, handlers, count, event->payload, payload);
        while (count--) IUnknown_Release(handlers[count]);
        free(handlers);
        break;
    }
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
    NTSTATUS status;
    UINT32 payload;
    void *buffer;

    if (!(params.buffer = malloc(WV2_INLINE_MAX))) return 1;
    params.size = WV2_INLINE_MAX;

    while ((status = UNIX_CALL(helper_recv, &params)) == STATUS_SUCCESS || status == STATUS_BUFFER_TOO_SMALL)
    {
        payload = params.header.size - sizeof(params.header);
        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            if (!(buffer = realloc(params.buffer, payload))) break;
            params.buffer = buffer;
            params.size = payload;
            continue;
        }
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

    if (!view || !view->created) return DefWindowProcW(hwnd, msg, wparam, lparam);

    switch (msg)
    {
    case WM_MOUSEMOVE:
        if (!view->tracking_mouse)
        {
            TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd};
            view->tracking_mouse = TrackMouseEvent(&tme);
        }
        send_mouse(view, WV2_MOUSE_MOVE, (short)LOWORD(lparam), (short)HIWORD(lparam), 0, 0, wparam, 0, 0);
        return 0;
    case WM_MOUSELEAVE:
        view->tracking_mouse = FALSE;
        send_mouse(view, WV2_MOUSE_LEAVE, 0, 0, 0, 0, 0, 0, 0);
        return 0;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
        if (GetFocus() != hwnd) SetFocus(hwnd);
        SetCapture(hwnd);
        send_mouse(view, WV2_MOUSE_DOWN, (short)LOWORD(lparam), (short)HIWORD(lparam),
                   msg <= WM_LBUTTONDBLCLK ? 0 : msg <= WM_RBUTTONDBLCLK ? 2 : 1,
                   (msg == WM_LBUTTONDBLCLK || msg == WM_RBUTTONDBLCLK || msg == WM_MBUTTONDBLCLK) ? 2 : 1,
                   wparam, 0, 0);
        return 0;
    case WM_LBUTTONUP: case WM_MBUTTONUP: case WM_RBUTTONUP:
        if (!(wparam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON))) ReleaseCapture();
        send_mouse(view, WV2_MOUSE_UP, (short)LOWORD(lparam), (short)HIWORD(lparam),
                   msg == WM_LBUTTONUP ? 0 : msg == WM_RBUTTONUP ? 2 : 1, 1, wparam, 0, 0);
        return 0;
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    {
        POINT pt = {(short)LOWORD(lparam), (short)HIWORD(lparam)};
        int delta = GET_WHEEL_DELTA_WPARAM(wparam);
        ScreenToClient(hwnd, &pt);
        send_mouse(view, WV2_MOUSE_WHEEL, pt.x, pt.y, 0, 0, GET_KEYSTATE_WPARAM(wparam),
                   msg == WM_MOUSEHWHEEL ? delta : 0, msg == WM_MOUSEWHEEL ? delta : 0);
        return 0;
    }
    case WM_KEYDOWN: case WM_KEYUP: case WM_CHAR:
        send_key(view, msg, wparam, lparam);
        return 0;
    case WM_SYSKEYDOWN: case WM_SYSKEYUP: case WM_SYSCHAR:
        send_key(view, msg, wparam, lparam);
        break;  /* keep Alt+F4, Alt+Space etc. working */
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    {
        struct wv2_flag flag = {msg == WM_SETFOCUS, 0};
        helper_send(view, WV2_CMD_FOCUS, &flag, sizeof(flag));
        return 0;
    }
    case WM_SETCURSOR:
        if (LOWORD(lparam) == HTCLIENT)
        {
            SetCursor(view->cursor);
            return TRUE;
        }
        break;
    case WM_GETDLGCODE:
        return DLGC_WANTALLKEYS | DLGC_WANTARROWS | DLGC_WANTCHARS | DLGC_WANTTAB;
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

static void complete_pending_scripts(struct view *view)
{
    struct pending_script *script, *next;

    LIST_FOR_EACH_ENTRY_SAFE(script, next, &view->pending_scripts, struct pending_script, entry)
    {
        list_remove(&script->entry);
        ICoreWebView2ExecuteScriptCompletedHandler_Invoke(script->handler, E_ABORT, NULL);
        ICoreWebView2ExecuteScriptCompletedHandler_Release(script->handler);
        free(script);
    }
}

static void close_view(struct view *view)
{
    unsigned int i;

    if (view->closed) return;
    view->closed = TRUE;
    complete_pending_scripts(view);
    handler_list_clear(&view->web_message_handlers);
    handler_list_clear(&view->web_resource_handlers);
    for (i = 0; i < EVENT_COUNT; i++) handler_list_clear(&view->events[i]);
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
    /* Drop the reference owned by the view; may destroy the controller (and release the view). */
    ICoreWebView2Controller_Release(view->controller);
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

static HRESULT STDMETHODCALLTYPE controller_MoveFocus(ICoreWebView2Controller4 *iface,
        COREWEBVIEW2_MOVE_FOCUS_REASON reason)
{
    struct view *view = view_from_controller(iface);

    TRACE("iface %p, reason %d.\n", iface, reason);
    if (view->closed) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    if (view->hwnd) SetFocus(view->hwnd);
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

static HRESULT STDMETHODCALLTYPE webview_ExecuteScript(ICoreWebView2_29 *iface, LPCWSTR script,
        ICoreWebView2ExecuteScriptCompletedHandler *handler)
{
    static LONG64 next_id;
    struct view *view = view_from_webview(iface);
    struct wv2_script info = {InterlockedIncrement64(&next_id)};
    struct pending_script *pending = NULL;
    HRESULT hr;

    TRACE("iface %p, script %s, handler %p.\n", iface, debugstr_w(script), handler);
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    if (!script) return E_INVALIDARG;
    if (handler && helper)
    {
        if (!(pending = malloc(sizeof(*pending)))) return E_OUTOFMEMORY;
        pending->id = info.id;
        pending->handler = handler;
        ICoreWebView2ExecuteScriptCompletedHandler_AddRef(handler);
        list_add_tail(&view->pending_scripts, &pending->entry);
    }
    if (FAILED(hr = helper_send_string(view, WV2_CMD_EXECUTE_SCRIPT, &info, sizeof(info), script)) && pending)
    {
        list_remove(&pending->entry);
        ICoreWebView2ExecuteScriptCompletedHandler_Release(handler);
        free(pending);
    }
    return hr;
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

static HRESULT STDMETHODCALLTYPE webview_add_WebMessageReceived(ICoreWebView2_29 *iface,
        ICoreWebView2WebMessageReceivedEventHandler *handler, EventRegistrationToken *token)
{
    struct view *view = view_from_webview(iface);

    TRACE("iface %p, handler %p, token %p.\n", iface, handler, token);
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    return handler_list_add(&view->web_message_handlers, (IUnknown *)handler, token);
}

static HRESULT STDMETHODCALLTYPE webview_remove_WebMessageReceived(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    struct view *view = view_from_webview(iface);

    TRACE("iface %p, token %s.\n", iface, wine_dbgstr_longlong(token.value));
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    return handler_list_remove(&view->web_message_handlers, token);
}

static HRESULT STDMETHODCALLTYPE webview_PostWebMessageAsJson(ICoreWebView2_29 *iface, LPCWSTR json)
{
    struct view *view = view_from_webview(iface);

    TRACE("iface %p, json %s.\n", iface, debugstr_w(json));
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    if (!json) return E_INVALIDARG;
    return helper_send_string(view, WV2_CMD_POST_WEB_MESSAGE, NULL, 0, json);
}

static HRESULT STDMETHODCALLTYPE webview_PostWebMessageAsString(ICoreWebView2_29 *iface, LPCWSTR str)
{
    struct view *view = view_from_webview(iface);
    WCHAR *json;
    HRESULT hr;

    TRACE("iface %p, str %s.\n", iface, debugstr_w(str));
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    if (!str) return E_INVALIDARG;
    if (!(json = json_quote(str))) return E_OUTOFMEMORY;
    hr = helper_send_string(view, WV2_CMD_POST_WEB_MESSAGE, NULL, 0, json);
    free(json);
    return hr;
}

static HRESULT STDMETHODCALLTYPE webview_add_WebResourceRequested(ICoreWebView2_29 *iface,
        ICoreWebView2WebResourceRequestedEventHandler *handler, EventRegistrationToken *token)
{
    struct view *view = view_from_webview(iface);

    TRACE("iface %p, handler %p, token %p.\n", iface, handler, token);
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    return handler_list_add(&view->web_resource_handlers, (IUnknown *)handler, token);
}

static HRESULT STDMETHODCALLTYPE webview_remove_WebResourceRequested(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    struct view *view = view_from_webview(iface);

    TRACE("iface %p, token %s.\n", iface, wine_dbgstr_longlong(token.value));
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    return handler_list_remove(&view->web_resource_handlers, token);
}

static HRESULT send_resource_filter(struct view *view, UINT32 type, LPCWSTR uri, COREWEBVIEW2_WEB_RESOURCE_CONTEXT context)
{
    struct wv2_resource_filter filter = {context, 0};

    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    if (!uri) return E_INVALIDARG;
    return helper_send_string(view, type, &filter, sizeof(filter), uri);
}

static HRESULT STDMETHODCALLTYPE webview_AddWebResourceRequestedFilter(ICoreWebView2_29 *iface, LPCWSTR uri,
        COREWEBVIEW2_WEB_RESOURCE_CONTEXT context)
{
    TRACE("iface %p, uri %s, context %d.\n", iface, debugstr_w(uri), context);
    return send_resource_filter(view_from_webview(iface), WV2_CMD_ADD_RESOURCE_FILTER, uri, context);
}

static HRESULT STDMETHODCALLTYPE webview_RemoveWebResourceRequestedFilter(ICoreWebView2_29 *iface, LPCWSTR uri,
        COREWEBVIEW2_WEB_RESOURCE_CONTEXT context)
{
    TRACE("iface %p, uri %s, context %d.\n", iface, debugstr_w(uri), context);
    return send_resource_filter(view_from_webview(iface), WV2_CMD_REMOVE_RESOURCE_FILTER, uri, context);
}

static HRESULT STDMETHODCALLTYPE webview_AddWebResourceRequestedFilterWithRequestSourceKinds(ICoreWebView2_29 *iface,
        LPCWSTR uri, COREWEBVIEW2_WEB_RESOURCE_CONTEXT context, COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS kinds)
{
    TRACE("iface %p, uri %s, context %d, kinds %#x.\n", iface, debugstr_w(uri), context, kinds);
    return send_resource_filter(view_from_webview(iface), WV2_CMD_ADD_RESOURCE_FILTER, uri, context);
}

static HRESULT STDMETHODCALLTYPE webview_RemoveWebResourceRequestedFilterWithRequestSourceKinds(ICoreWebView2_29 *iface,
        LPCWSTR uri, COREWEBVIEW2_WEB_RESOURCE_CONTEXT context, COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS kinds)
{
    TRACE("iface %p, uri %s, context %d, kinds %#x.\n", iface, debugstr_w(uri), context, kinds);
    return send_resource_filter(view_from_webview(iface), WV2_CMD_REMOVE_RESOURCE_FILTER, uri, context);
}

#define DEFINE_EVENT(name, event, handler_type) \
static HRESULT STDMETHODCALLTYPE webview_add_##name(ICoreWebView2_29 *iface, handler_type *handler, \
        EventRegistrationToken *token) \
{ \
    struct view *view = view_from_webview(iface); \
    TRACE("iface %p, handler %p, token %p.\n", iface, handler, token); \
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE); \
    return handler_list_add(&view->events[event], (IUnknown *)handler, token); \
} \
static HRESULT STDMETHODCALLTYPE webview_remove_##name(ICoreWebView2_29 *iface, EventRegistrationToken token) \
{ \
    struct view *view = view_from_webview(iface); \
    TRACE("iface %p, token %s.\n", iface, wine_dbgstr_longlong(token.value)); \
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE); \
    return handler_list_remove(&view->events[event], token); \
}

DEFINE_EVENT(NavigationStarting, EVENT_NAVIGATION_STARTING, ICoreWebView2NavigationStartingEventHandler)
DEFINE_EVENT(ContentLoading, EVENT_CONTENT_LOADING, ICoreWebView2ContentLoadingEventHandler)
DEFINE_EVENT(SourceChanged, EVENT_SOURCE_CHANGED, ICoreWebView2SourceChangedEventHandler)
DEFINE_EVENT(HistoryChanged, EVENT_HISTORY_CHANGED, ICoreWebView2HistoryChangedEventHandler)
DEFINE_EVENT(DOMContentLoaded, EVENT_DOM_CONTENT_LOADED, ICoreWebView2DOMContentLoadedEventHandler)
DEFINE_EVENT(NavigationCompleted, EVENT_NAVIGATION_COMPLETED, ICoreWebView2NavigationCompletedEventHandler)
DEFINE_EVENT(DocumentTitleChanged, EVENT_DOCUMENT_TITLE_CHANGED, ICoreWebView2DocumentTitleChangedEventHandler)

static HRESULT return_string(const WCHAR *str, LPWSTR *out)
{
    if (!out) return E_POINTER;
    return (*out = co_strdupW(str ? str : L"")) ? S_OK : E_OUTOFMEMORY;
}

static HRESULT STDMETHODCALLTYPE webview_get_Source(ICoreWebView2_29 *iface, LPWSTR *uri)
{
    struct view *view = view_from_webview(iface);

    TRACE("iface %p, uri %p.\n", iface, uri);
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    return return_string(view->source ? view->source : L"about:blank", uri);
}

static HRESULT STDMETHODCALLTYPE webview_get_DocumentTitle(ICoreWebView2_29 *iface, LPWSTR *title)
{
    struct view *view = view_from_webview(iface);

    TRACE("iface %p, title %p.\n", iface, title);
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    return return_string(view->title, title);
}

static HRESULT STDMETHODCALLTYPE webview_get_CanGoBack(ICoreWebView2_29 *iface, BOOL *value)
{
    struct view *view = view_from_webview(iface);

    TRACE("iface %p, value %p.\n", iface, value);
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    if (!value) return E_POINTER;
    *value = view->can_go_back;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE webview_get_CanGoForward(ICoreWebView2_29 *iface, BOOL *value)
{
    struct view *view = view_from_webview(iface);

    TRACE("iface %p, value %p.\n", iface, value);
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    if (!value) return E_POINTER;
    *value = view->can_go_forward;
    return S_OK;
}

#define DEFINE_SIMPLE_COMMAND(name, command) \
static HRESULT STDMETHODCALLTYPE webview_##name(ICoreWebView2_29 *iface) \
{ \
    struct view *view = view_from_webview(iface); \
    TRACE("iface %p.\n", iface); \
    if (!view) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE); \
    return helper_send(view, command, NULL, 0); \
}

DEFINE_SIMPLE_COMMAND(GoBack, WV2_CMD_GO_BACK)
DEFINE_SIMPLE_COMMAND(GoForward, WV2_CMD_GO_FORWARD)
DEFINE_SIMPLE_COMMAND(Reload, WV2_CMD_RELOAD)
DEFINE_SIMPLE_COMMAND(Stop, WV2_CMD_STOP)
DEFINE_SIMPLE_COMMAND(OpenDevToolsWindow, WV2_CMD_OPEN_DEVTOOLS)

/* ICoreWebView2WebMessageReceivedEventArgs */

struct web_message
{
    WCHAR *source;
    WCHAR *json;
};

static ICoreWebView2WebMessageReceivedEventArgs2Vtbl web_message_args_vtbl;
static struct stub_family web_message_args_family;

static struct web_message *web_message_from_args(ICoreWebView2WebMessageReceivedEventArgs2 *iface)
{
    return ((struct stub_object *)iface)->data;
}

static void web_message_args_destroy(struct stub_object *obj)
{
    struct web_message *msg = obj->data;

    free(msg->source);
    free(msg->json);
    free(msg);
}

static HRESULT STDMETHODCALLTYPE web_message_args_get_Source(ICoreWebView2WebMessageReceivedEventArgs2 *iface,
        LPWSTR *source)
{
    TRACE("iface %p, source %p.\n", iface, source);
    if (!source) return E_POINTER;
    return (*source = co_strdupW(web_message_from_args(iface)->source)) ? S_OK : E_OUTOFMEMORY;
}

static HRESULT STDMETHODCALLTYPE web_message_args_get_WebMessageAsJson(ICoreWebView2WebMessageReceivedEventArgs2 *iface,
        LPWSTR *json)
{
    TRACE("iface %p, json %p.\n", iface, json);
    if (!json) return E_POINTER;
    return (*json = co_strdupW(web_message_from_args(iface)->json)) ? S_OK : E_OUTOFMEMORY;
}

static HRESULT STDMETHODCALLTYPE web_message_args_TryGetWebMessageAsString(ICoreWebView2WebMessageReceivedEventArgs2 *iface,
        LPWSTR *str)
{
    TRACE("iface %p, str %p.\n", iface, str);
    if (!str) return E_POINTER;
    /* Succeeds only when the page posted a string. */
    if (!(*str = json_unquote(web_message_from_args(iface)->json))) return E_INVALIDARG;
    return S_OK;
}

static void handle_web_message(struct view *view, const BYTE *payload, UINT32 size)
{
    const struct wv2_web_message *info = (const void *)payload;
    ICoreWebView2WebMessageReceivedEventHandler **handlers;
    struct stub_object *args;
    struct web_message *msg;
    const char *source;
    unsigned int i, count;

    if (size < sizeof(*info) || size - sizeof(*info) < info->source_size || !view->webview) return;
    source = (const char *)(info + 1);
    if (!(msg = calloc(1, sizeof(*msg)))) return;
    msg->source = strdupAtoW_n(source, info->source_size);
    msg->json = strdupAtoW_n(source + info->source_size, size - sizeof(*info) - info->source_size);
    if (!msg->source || !msg->json || !(args = stub_create(&web_message_args_family)))
    {
        free(msg->source);
        free(msg->json);
        free(msg);
        return;
    }
    args->data = msg;
    args->destroy = web_message_args_destroy;
    TRACE("view %u: web message from %s: %s\n", view->id, debugstr_w(msg->source), debugstr_w(msg->json));

    handlers = (ICoreWebView2WebMessageReceivedEventHandler **)handler_list_snapshot(&view->web_message_handlers, &count);
    for (i = 0; i < count; i++)
    {
        ICoreWebView2WebMessageReceivedEventHandler_Invoke(handlers[i], (ICoreWebView2 *)view->webview,
                (ICoreWebView2WebMessageReceivedEventArgs *)args);
        ICoreWebView2WebMessageReceivedEventHandler_Release(handlers[i]);
    }
    free(handlers);
    ICoreWebView2WebMessageReceivedEventArgs_Release((ICoreWebView2WebMessageReceivedEventArgs *)args);
}

static void handle_script_result(struct view *view, const BYTE *payload, UINT32 size)
{
    const struct wv2_script *info = (const void *)payload;
    struct pending_script *script;
    WCHAR *json;

    if (size < sizeof(*info)) return;
    LIST_FOR_EACH_ENTRY(script, &view->pending_scripts, struct pending_script, entry)
    {
        if (script->id != info->id) continue;
        list_remove(&script->entry);
        json = strdupAtoW_n((const char *)(info + 1), size - sizeof(*info));
        TRACE("view %u: script %s result %s\n", view->id, wine_dbgstr_longlong(info->id), debugstr_w(json));
        ICoreWebView2ExecuteScriptCompletedHandler_Invoke(script->handler, S_OK, json ? json : L"null");
        ICoreWebView2ExecuteScriptCompletedHandler_Release(script->handler);
        free(script);
        free(json);
        return;
    }
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
    list_init(&view->pending_scripts);
    view->cursor = LoadCursorW(NULL, (const WCHAR *)IDC_ARROW);
    view->parent = parent;
    view->async_hwnd = webview2_get_async_window();
    /* Like the real runtime, the view keeps the controller alive until Close() (applications such
     * as Wails do not AddRef the controller they receive); released in close_view(). */
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
            return S_OK;    /* handler runs on WV2_EVT_VIEW_CREATED */
        }
    }

    /* Skeleton mode (no helper): complete right away, asynchronously. */
    view_addref(view);
    webview2_queue_async(controller_created_skeleton_cb, view);
    return S_OK;
}

void controller_init(void)
{
    WNDCLASSW cls = {0};

    cls.style = CS_DBLCLKS;
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
    controller_vtbl.MoveFocus = controller_MoveFocus;
    controller_family = stub_family_ICoreWebView2Controller4;
    controller_family.vtbl = &controller_vtbl;

    webview_vtbl = *(const ICoreWebView2_29Vtbl *)stub_family_ICoreWebView2_29.vtbl;
    webview_vtbl.Navigate = webview_Navigate;
    webview_vtbl.NavigateToString = webview_NavigateToString;
    webview_vtbl.ExecuteScript = webview_ExecuteScript;
    webview_vtbl.AddScriptToExecuteOnDocumentCreated = webview_AddScriptToExecuteOnDocumentCreated;
    webview_vtbl.add_WebMessageReceived = webview_add_WebMessageReceived;
    webview_vtbl.remove_WebMessageReceived = webview_remove_WebMessageReceived;
    webview_vtbl.PostWebMessageAsJson = webview_PostWebMessageAsJson;
    webview_vtbl.PostWebMessageAsString = webview_PostWebMessageAsString;
    webview_vtbl.add_WebResourceRequested = webview_add_WebResourceRequested;
    webview_vtbl.remove_WebResourceRequested = webview_remove_WebResourceRequested;
    webview_vtbl.AddWebResourceRequestedFilter = webview_AddWebResourceRequestedFilter;
    webview_vtbl.RemoveWebResourceRequestedFilter = webview_RemoveWebResourceRequestedFilter;
    webview_vtbl.AddWebResourceRequestedFilterWithRequestSourceKinds = webview_AddWebResourceRequestedFilterWithRequestSourceKinds;
    webview_vtbl.RemoveWebResourceRequestedFilterWithRequestSourceKinds = webview_RemoveWebResourceRequestedFilterWithRequestSourceKinds;
    webview_vtbl.add_NavigationStarting = webview_add_NavigationStarting;
    webview_vtbl.remove_NavigationStarting = webview_remove_NavigationStarting;
    webview_vtbl.add_ContentLoading = webview_add_ContentLoading;
    webview_vtbl.remove_ContentLoading = webview_remove_ContentLoading;
    webview_vtbl.add_SourceChanged = webview_add_SourceChanged;
    webview_vtbl.remove_SourceChanged = webview_remove_SourceChanged;
    webview_vtbl.add_HistoryChanged = webview_add_HistoryChanged;
    webview_vtbl.remove_HistoryChanged = webview_remove_HistoryChanged;
    webview_vtbl.add_DOMContentLoaded = webview_add_DOMContentLoaded;
    webview_vtbl.remove_DOMContentLoaded = webview_remove_DOMContentLoaded;
    webview_vtbl.add_NavigationCompleted = webview_add_NavigationCompleted;
    webview_vtbl.remove_NavigationCompleted = webview_remove_NavigationCompleted;
    webview_vtbl.add_DocumentTitleChanged = webview_add_DocumentTitleChanged;
    webview_vtbl.remove_DocumentTitleChanged = webview_remove_DocumentTitleChanged;
    webview_vtbl.get_Source = webview_get_Source;
    webview_vtbl.get_DocumentTitle = webview_get_DocumentTitle;
    webview_vtbl.get_CanGoBack = webview_get_CanGoBack;
    webview_vtbl.get_CanGoForward = webview_get_CanGoForward;
    webview_vtbl.GoBack = webview_GoBack;
    webview_vtbl.GoForward = webview_GoForward;
    webview_vtbl.Reload = webview_Reload;
    webview_vtbl.Stop = webview_Stop;
    webview_vtbl.OpenDevToolsWindow = webview_OpenDevToolsWindow;
    webview_family = stub_family_ICoreWebView2_29;
    webview_family.vtbl = &webview_vtbl;

    web_message_args_vtbl = *(const ICoreWebView2WebMessageReceivedEventArgs2Vtbl *)
            stub_family_ICoreWebView2WebMessageReceivedEventArgs2.vtbl;
    web_message_args_vtbl.get_Source = web_message_args_get_Source;
    web_message_args_vtbl.get_WebMessageAsJson = web_message_args_get_WebMessageAsJson;
    web_message_args_vtbl.TryGetWebMessageAsString = web_message_args_TryGetWebMessageAsString;
    web_message_args_family = stub_family_ICoreWebView2WebMessageReceivedEventArgs2;
    web_message_args_family.vtbl = &web_message_args_vtbl;
}
