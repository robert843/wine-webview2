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
#include "uxtheme.h"
#include "objbase.h"
#include "webview2.h"
#include "webview2_private.h"
#include "unixlib.h"
#include "proxy.h"
#include "proxy_gen.h"

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
static DWORD browser_process_id;       /* msedgewebview2.exe placeholder, see start_placeholder() */
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
    BOOL closed;
    IUnknown *controller;   /* proxy, reference owned by the view until Close() */
    COREWEBVIEW2_COLOR background;
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

    if (!helper) return E_FAIL;
    if (UNIX_CALL(helper_send, &params))
    {
        WARN("failed to send message %u\n", type);
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
    case WV2_EVT_CURSOR:
        if (payload >= sizeof(struct wv2_flag))
            set_view_cursor(view, ((const struct wv2_flag *)event->payload)->value);
        break;
    case WV2_EVT_LOAD_STATE:
        if (payload >= sizeof(struct wv2_flag))
            TRACE("view %u: %s\n", view->id, ((const struct wv2_flag *)event->payload)->value ? "loading" : "loaded");
        break;
    case WV2_EVT_ERROR:
        ERR("view %u: helper error %s\n", view->id, debugstr_an((const char *)event->payload, payload));
        break;
    default:
        TRACE("view %u: ignored event %#x\n", view->id, event->header.type);
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
        /* Generic calls: replies wake the waiting thread directly; invokes go to the object's thread. */
        switch (params.header.type)
        {
        case WV2_EVT_REPLY: proxy_handle_reply(params.buffer, payload); continue;
        case WV2_EVT_INVOKE: proxy_handle_invoke(params.buffer, payload); continue;
        case WV2_EVT_RELEASE_APP: proxy_handle_release_app(params.buffer, payload); continue;
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
    proxy_connection_lost();
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

static void start_placeholder(void);

static BOOL WINAPI start_helper_once(INIT_ONCE *once, void *param, void **context)
{
    struct helper_start_params params = {WV2_API_HASH};
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
    start_placeholder();
    return TRUE;
}

/* Starts msedgewebview2.exe (from the runtime's version folder, two levels above the DLL) as a
 * stand-in browser process: applications open the process reported by get_BrowserProcessId. */
static void start_placeholder(void)
{
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits;
    WCHAR path[MAX_PATH], cmdline[MAX_PATH + 64], *p;
    PROCESS_INFORMATION pi;
    STARTUPINFOW si = {sizeof(si)};
    HANDLE job;
    int i;

    memset(&limits, 0, sizeof(limits));
    GetModuleFileNameW(webview2_instance, path, ARRAY_SIZE(path));
    for (i = 0; i < 3 && (p = wcsrchr(path, '\\')); i++) *p = 0;   /* ...\<version>\EBWebView\x64\dll */
    wcscat(path, L"\\msedgewebview2.exe");
    swprintf(cmdline, ARRAY_SIZE(cmdline), L"\"%s\" --wine-webview2-placeholder", path);
    if (!CreateProcessW(path, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED | CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        WARN("cannot start %s, error %lu\n", debugstr_w(path), GetLastError());
        return;
    }
    if ((job = CreateJobObjectW(NULL, NULL)))
    {
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits));
        AssignProcessToJobObject(job, pi.hProcess);     /* handle kept open: dies with us */
    }
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    browser_process_id = pi.dwProcessId;
    TRACE("placeholder browser process %lu\n", browser_process_id);
}

BOOL helper_ensure_started(void)
{
    InitOnceExecuteOnce(&helper_once, start_helper_once, NULL, NULL);
    return helper != 0;
}

/*
 * Host window: child of the application window showing the latest frame.
 */

static const WCHAR host_class_name[] = L"WineWebView2Host";

/* The helper renders with a transparent background; DefaultBackgroundColor goes below the
 * premultiplied BGRA frame here: the colour itself, or the parent's content when transparent
 * (WebView2 uses DirectComposition for the latter; same visible result for child windows). */
static void paint_view(struct view *view, HDC hdc, const RECT *rect)
{
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    int width = rect->right - rect->left, height = rect->bottom - rect->top;
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bitmap = CreateCompatibleBitmap(hdc, width, height), old;
    RECT local = {0, 0, width, height};

    old = SelectObject(mem, bitmap);
    if (view->background.A)
    {
        HBRUSH brush = CreateSolidBrush(RGB(view->background.R, view->background.G, view->background.B));
        FillRect(mem, &local, brush);
        DeleteObject(brush);
    }
    else
    {
        SetViewportOrgEx(mem, -rect->left, -rect->top, NULL);
        DrawThemeParentBackground(view->hwnd, mem, (RECT *)rect);
        SetViewportOrgEx(mem, 0, 0, NULL);
    }
    if (!view->dc)
    {
        BitBlt(hdc, rect->left, rect->top, width, height, mem, 0, 0, SRCCOPY);
        goto done;
    }
    width = max(0, min(width, (int)view->dib_width - rect->left));
    height = max(0, min(height, (int)view->dib_height - rect->top));
    if (width && height)
        GdiAlphaBlend(mem, 0, 0, width, height, view->dc, rect->left, rect->top, width, height, blend);
    BitBlt(hdc, rect->left, rect->top, local.right, local.bottom, mem, 0, 0, SRCCOPY);
done:
    SelectObject(mem, old);
    DeleteObject(bitmap);
    DeleteDC(mem);
}

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
        if (view) paint_view(view, hdc, &ps.rcPaint);
        else FillRect(hdc, &ps.rcPaint, GetStockObject(WHITE_BRUSH));
        EndPaint(hwnd, &ps);
        return 0;
    case WM_NCDESTROY:
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        break;
    }

    if (!view || view->closed || !view->controller) return DefWindowProcW(hwnd, msg, wparam, lparam);

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
 * ICoreWebView2Controller: proxy of the helper's controller; window-related methods are done
 * here by the host and never reach the helper.
 */

static ICoreWebView2Controller4Vtbl controller_vtbl;
static ICoreWebView2_29Vtbl webview_vtbl;

static HRESULT (STDMETHODCALLTYPE *proxy_SetVirtualHostNameToFolderMapping)(ICoreWebView2_29 *iface,
        LPCWSTR host, LPCWSTR folder, COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND kind);

/* The helper reads the files natively: pass the folder as a unix path. */
static HRESULT STDMETHODCALLTYPE webview_SetVirtualHostNameToFolderMapping(ICoreWebView2_29 *iface,
        LPCWSTR host, LPCWSTR folder, COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND kind)
{
    WCHAR *unix_folder;
    char *unix_name;
    HRESULT hr;
    int len;

    TRACE("iface %p, host %s, folder %s, kind %d.\n", iface, debugstr_w(host), debugstr_w(folder), kind);
    if (!host || !folder) return E_INVALIDARG;
    if (!(unix_name = wine_get_unix_file_name(folder))) return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
    len = MultiByteToWideChar(CP_UNIXCP, 0, unix_name, -1, NULL, 0);
    if (!(unix_folder = malloc(len * sizeof(WCHAR))))
    {
        HeapFree(GetProcessHeap(), 0, unix_name);
        return E_OUTOFMEMORY;
    }
    MultiByteToWideChar(CP_UNIXCP, 0, unix_name, -1, unix_folder, len);
    HeapFree(GetProcessHeap(), 0, unix_name);
    hr = proxy_SetVirtualHostNameToFolderMapping(iface, host, unix_folder, kind);
    free(unix_folder);
    return hr;
}

static HRESULT STDMETHODCALLTYPE webview_get_BrowserProcessId(ICoreWebView2_29 *iface, UINT32 *pid)
{
    TRACE("iface %p, pid %p.\n", iface, pid);
    if (!pid) return E_POINTER;
    *pid = browser_process_id;
    return S_OK;
}
static HRESULT (STDMETHODCALLTYPE *proxy_controller_Close)(ICoreWebView2Controller4 *iface);

static struct view *view_from_controller(ICoreWebView2Controller4 *iface)
{
    return proxy_get_host_data((IUnknown *)iface);
}

static void controller_host_destroy(void *data)
{
    view_release(data);
}

static void close_view(struct view *view)
{
    IUnknown *controller = view->controller;

    if (view->closed) return;
    view->closed = TRUE;
    if (view->hwnd)
    {
        DestroyWindow(view->hwnd);
        view->hwnd = NULL;
    }
    view->controller = NULL;
    if (controller) IUnknown_Release(controller);   /* may destroy the proxy and release the view */
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
    send_size(view);
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
    send_visible(view);
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

static HRESULT STDMETHODCALLTYPE controller_NotifyParentWindowPositionChanged(ICoreWebView2Controller4 *iface)
{
    TRACE("iface %p.\n", iface);
    return S_OK;    /* the host window is a real child window and moves with its parent */
}

static UINT32 color_to_argb(COREWEBVIEW2_COLOR color)
{
    return (UINT32)color.A << 24 | (UINT32)color.R << 16 | (UINT32)color.G << 8 | color.B;
}

static HRESULT STDMETHODCALLTYPE controller_get_DefaultBackgroundColor(ICoreWebView2Controller4 *iface,
        COREWEBVIEW2_COLOR *color)
{
    struct view *view = view_from_controller(iface);

    TRACE("iface %p, color %p.\n", iface, color);
    if (!color) return E_POINTER;
    *color = view->background;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE controller_put_DefaultBackgroundColor(ICoreWebView2Controller4 *iface,
        COREWEBVIEW2_COLOR color)
{
    struct view *view = view_from_controller(iface);

    TRACE("iface %p, color ARGB %08x.\n", iface, color_to_argb(color));
    if (view->closed) return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    /* WebView2 only supports fully opaque or fully transparent backgrounds. */
    if (color.A != 0 && color.A != 255) return E_INVALIDARG;
    view->background = color;
    if (view->hwnd) InvalidateRect(view->hwnd, NULL, FALSE);   /* composited in paint_view() */
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
    HRESULT hr;

    TRACE("iface %p.\n", iface);
    if (view->closed) return S_OK;
    hr = proxy_controller_Close(iface);     /* the helper closes the browser */
    close_view(view);
    return hr;
}

static COREWEBVIEW2_COLOR default_background_color(void)
{
    COREWEBVIEW2_COLOR color = {255, 255, 255, 255};
    WCHAR buf[16], *end;
    UINT32 value;

    if (!GetEnvironmentVariableW(L"WEBVIEW2_DEFAULT_BACKGROUND_COLOR", buf, ARRAY_SIZE(buf))) return color;
    value = wcstoul(buf, &end, 16);
    if (*end) return color;
    if (wcslen(buf) <= 6) value |= 0xff000000;
    color.A = value >> 24;
    color.R = value >> 16;
    color.G = value >> 8;
    color.B = value;
    if (color.A != 255) color.A = 0;
    return color;
}

/*
 * Controller creation (from the environment's host override).
 */

struct controller_created_ctx
{
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *handler;
    IUnknown *controller;
    HRESULT hr;
};

static void controller_created_cb(void *arg)
{
    struct controller_created_ctx *ctx = arg;
    HRESULT hr;

    TRACE("invoking controller completed handler %p, hr %#lx, controller %p\n", ctx->handler, ctx->hr, ctx->controller);
    hr = ICoreWebView2CreateCoreWebView2ControllerCompletedHandler_Invoke(ctx->handler, ctx->hr,
            (ICoreWebView2Controller *)ctx->controller);
    TRACE("handler returned %#lx\n", hr);
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandler_Release(ctx->handler);
    if (ctx->controller) IUnknown_Release(ctx->controller);
    free(ctx);
}

HRESULT controller_create(IUnknown *environment, HWND parent,
                          ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *handler)
{
    struct controller_created_ctx *ctx;
    struct wv2_buffer args = {0};
    struct wv2_reader ret;
    struct view *view;

    if (!handler) return E_POINTER;
    if (!IsWindow(parent)) return E_INVALIDARG;
    if (!(view = calloc(1, sizeof(*view)))) return E_OUTOFMEMORY;
    if (!(ctx = calloc(1, sizeof(*ctx))))
    {
        free(view);
        return E_OUTOFMEMORY;
    }

    view->ref = 1;          /* released when the controller proxy goes away */
    view->parent = parent;
    view->async_hwnd = webview2_get_async_window();
    view->visible = TRUE;
    view->cursor = LoadCursorW(NULL, (const WCHAR *)IDC_ARROW);
    view->background = default_background_color();
    GetClientRect(parent, &view->bounds);

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

    wv2_put_iface(&args, environment);
    wv2_put_u32(&args, view->id);
    wv2_put_u32(&args, max(1, view->bounds.right - view->bounds.left));
    wv2_put_u32(&args, max(1, view->bounds.bottom - view->bounds.top));
    if (SUCCEEDED(ctx->hr = proxy_call_root(WV2_ROOT_CREATE_CONTROLLER, &args, &ret)) &&
        !(view->controller = wv2_get_iface(&ret)))
        ctx->hr = E_FAIL;
    wv2_reader_free(&ret);
    TRACE("view %u, parent %p, bounds %s, host window %p, controller %p, hr %#lx\n", view->id, parent,
          wine_dbgstr_rect(&view->bounds), view->hwnd, view->controller, ctx->hr);

    if (view->controller)
    {
        proxy_set_host_data(view->controller, view, controller_host_destroy);
        ctx->controller = view->controller;
        IUnknown_AddRef(ctx->controller);
    }
    else
    {
        if (view->hwnd) DestroyWindow(view->hwnd);
        view_release(view);
    }

    ctx->handler = handler;
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandler_AddRef(handler);
    webview2_queue_async(controller_created_cb, ctx);
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

    controller_vtbl = *(const ICoreWebView2Controller4Vtbl *)proxy_get_vtbl(FAMILY_ICoreWebView2Controller4);
    proxy_controller_Close = controller_vtbl.Close;
    controller_vtbl.get_Bounds = controller_get_Bounds;
    controller_vtbl.put_Bounds = controller_put_Bounds;
    controller_vtbl.get_IsVisible = controller_get_IsVisible;
    controller_vtbl.put_IsVisible = controller_put_IsVisible;
    controller_vtbl.get_ParentWindow = controller_get_ParentWindow;
    controller_vtbl.put_ParentWindow = controller_put_ParentWindow;
    controller_vtbl.NotifyParentWindowPositionChanged = controller_NotifyParentWindowPositionChanged;
    controller_vtbl.get_DefaultBackgroundColor = controller_get_DefaultBackgroundColor;
    controller_vtbl.put_DefaultBackgroundColor = controller_put_DefaultBackgroundColor;
    controller_vtbl.MoveFocus = controller_MoveFocus;
    controller_vtbl.Close = controller_Close;
    proxy_set_host_vtbl(FAMILY_ICoreWebView2Controller4, &controller_vtbl);

    webview_vtbl = *(const ICoreWebView2_29Vtbl *)proxy_get_vtbl(FAMILY_ICoreWebView2_29);
    webview_vtbl.get_BrowserProcessId = webview_get_BrowserProcessId;
    proxy_SetVirtualHostNameToFolderMapping = webview_vtbl.SetVirtualHostNameToFolderMapping;
    webview_vtbl.SetVirtualHostNameToFolderMapping = webview_SetVirtualHostNameToFolderMapping;
    proxy_set_host_vtbl(FAMILY_ICoreWebView2_29, &webview_vtbl);
}
