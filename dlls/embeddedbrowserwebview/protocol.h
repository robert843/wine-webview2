/*
 * Wire protocol between EmbeddedBrowserWebView (unix side) and wine-webview2-helper.
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

/*
 * Transport: AF_UNIX SOCK_SEQPACKET socketpair (message boundaries preserved), helper end
 * passed as fd given by "--ipc-fd=N". One message = one wv2_msg_header + payload, all
 * integers little-endian (host order), strings UTF-8 without terminator (length = payload
 * size minus fixed part). File descriptors travel as SCM_RIGHTS ancillary data; large payloads
 * travel in a memfd (see WV2_INLINE_MAX), so messages have no practical size limit.
 *
 * Handshake: DLL sends WV2_CMD_HELLO, helper answers WV2_EVT_HELLO with its version; on
 * mismatch the DLL closes the socket. Any change of a message layout bumps the version.
 *
 * Frames: the helper owns a memfd with WV2_FRAME_SLOTS slots of stride*height bytes (BGRA,
 * premultiplied alpha, top-down) and sends it with WV2_EVT_FRAME_BUFFER whenever the size
 * changes. WV2_EVT_FRAME_READY names a slot; the helper does not write that slot again
 * until the DLL returns it with WV2_CMD_FRAME_DONE.
 *
 * Mirror: helper/src/protocol.rs — keep both in sync.
 */

#ifndef __WEBVIEW2_PROTOCOL_H
#define __WEBVIEW2_PROTOCOL_H

#define WV2_PROTOCOL_VERSION 6
#define WV2_FRAME_SLOTS      2
/* SOCK_SEQPACKET messages must fit the socket send buffer (net.core.wmem_default, usually
 * 212992 bytes). Payloads larger than WV2_INLINE_MAX therefore travel in a sealed memfd attached
 * as the first SCM_RIGHTS descriptor, with WV2_MSG_PAYLOAD_IN_FD set and only the header in the
 * datagram (header.size still counts header + payload). Same idea as Mojo's shared buffers. */
#define WV2_INLINE_MAX       (64 * 1024)
#define WV2_MAX_PAYLOAD      (256 * 1024 * 1024)
#define WV2_MSG_PAYLOAD_IN_FD 0x1

/* DLL -> helper */
enum wv2_command
{
    WV2_CMD_HELLO = 1,              /* wv2_hello */
    WV2_CMD_CREATE_VIEW,            /* wv2_view_size */
    WV2_CMD_CLOSE_VIEW,             /* (no payload) */
    WV2_CMD_RESIZE,                 /* wv2_view_size */
    WV2_CMD_SET_VISIBLE,            /* wv2_flag */
    WV2_CMD_NAVIGATE,               /* UTF-8 URL */
    WV2_CMD_NAVIGATE_TO_STRING,     /* UTF-8 HTML */
    WV2_CMD_EXECUTE_SCRIPT,         /* wv2_script + UTF-8 script */
    WV2_CMD_ADD_DOCUMENT_SCRIPT,    /* wv2_script + UTF-8 script (id = script id) */
    WV2_CMD_FRAME_DONE,             /* wv2_frame_done */
    WV2_CMD_SHUTDOWN,               /* (no payload) */
    WV2_CMD_POST_WEB_MESSAGE,       /* UTF-8 JSON, delivered to chrome.webview "message" listeners */
    WV2_CMD_MOUSE,                  /* wv2_mouse */
    WV2_CMD_KEY,                    /* wv2_key */
    WV2_CMD_FOCUS,                  /* wv2_flag */
    WV2_CMD_ADD_RESOURCE_FILTER,    /* wv2_resource_filter + UTF-8 URI pattern (* and ? wildcards) */
    WV2_CMD_REMOVE_RESOURCE_FILTER, /* wv2_resource_filter + UTF-8 URI pattern */
    WV2_CMD_RESOURCE_RESPONSE,      /* wv2_resource_response + UTF-8 reason + UTF-8 headers */
    WV2_CMD_RESOURCE_DATA,          /* wv2_resource_id + content bytes */
    WV2_CMD_RESOURCE_END,           /* wv2_resource_id */
    WV2_CMD_GO_BACK,                /* (no payload) */
    WV2_CMD_GO_FORWARD,             /* (no payload) */
    WV2_CMD_RELOAD,                 /* (no payload) */
    WV2_CMD_STOP,                   /* (no payload) */
    WV2_CMD_OPEN_DEVTOOLS,          /* (no payload) */
};

/* helper -> DLL */
enum wv2_event
{
    WV2_EVT_HELLO = 0x1001,         /* wv2_hello */
    WV2_EVT_VIEW_CREATED,           /* (no payload) */
    WV2_EVT_FRAME_BUFFER,           /* wv2_frame_buffer + memfd */
    WV2_EVT_FRAME_READY,            /* wv2_frame_ready */
    WV2_EVT_LOAD_STATE,             /* wv2_flag (1 = loading) */
    WV2_EVT_SCRIPT_RESULT,          /* wv2_script + UTF-8 JSON */
    WV2_EVT_ERROR,                  /* UTF-8 message */
    WV2_EVT_WEB_MESSAGE,            /* wv2_web_message + UTF-8 source URL + UTF-8 JSON */
    WV2_EVT_CURSOR,                 /* wv2_flag: cef_cursor_type_t */
    WV2_EVT_RESOURCE_REQUEST,       /* wv2_resource_request + URL + method + headers + body */
    WV2_EVT_NAVIGATION_STARTING,    /* wv2_navigation (a = user initiated, b = redirect) + UTF-8 URL */
    WV2_EVT_SOURCE_CHANGED,         /* UTF-8 URL of the main frame */
    WV2_EVT_CONTENT_LOADING,        /* wv2_navigation (a = error page) */
    WV2_EVT_NAVIGATION_COMPLETED,   /* wv2_navigation_completed */
    WV2_EVT_TITLE_CHANGED,          /* UTF-8 title */
    WV2_EVT_HISTORY_CHANGED,        /* wv2_navigation (a = can go back, b = can go forward) */
};

enum wv2_mouse_kind
{
    WV2_MOUSE_MOVE,
    WV2_MOUSE_DOWN,
    WV2_MOUSE_UP,
    WV2_MOUSE_WHEEL,
    WV2_MOUSE_LEAVE,
};

/* Modifier bits are cef_event_flags_t, buttons cef_mouse_button_type_t, key kinds
 * cef_key_event_type_t (RAWKEYDOWN 0, KEYDOWN 1, KEYUP 2, CHAR 3). */
#define WV2_FLAG_CAPS_LOCK_ON   0x0001
#define WV2_FLAG_SHIFT_DOWN     0x0002
#define WV2_FLAG_CONTROL_DOWN   0x0004
#define WV2_FLAG_ALT_DOWN       0x0008
#define WV2_FLAG_LEFT_BUTTON    0x0010
#define WV2_FLAG_MIDDLE_BUTTON  0x0020
#define WV2_FLAG_RIGHT_BUTTON   0x0040
#define WV2_FLAG_NUM_LOCK_ON    0x0100
#define WV2_FLAG_IS_KEY_PAD     0x0200
#define WV2_FLAG_IS_LEFT        0x0400
#define WV2_FLAG_IS_RIGHT       0x0800
#define WV2_FLAG_IS_REPEAT      0x2000

struct wv2_msg_header
{
    UINT32 type;
    UINT32 size;    /* whole message including this header */
    UINT32 view;    /* view id chosen by the DLL, 0 = global */
    UINT32 flags;   /* WV2_MSG_* */
};

struct wv2_hello
{
    UINT32 version;
    UINT32 pid;
};

struct wv2_view_size
{
    UINT32 width;
    UINT32 height;
    UINT32 scale_percent;   /* device scale factor * 100 */
    UINT32 reserved;
};

struct wv2_flag
{
    UINT32 value;
    UINT32 reserved;
};

struct wv2_script
{
    UINT64 id;
};

struct wv2_mouse
{
    UINT32 kind;            /* enum wv2_mouse_kind */
    INT32  x, y;
    UINT32 button;          /* 0 left, 1 middle, 2 right */
    UINT32 click_count;
    UINT32 modifiers;
    INT32  delta_x, delta_y;
};

struct wv2_key
{
    UINT32 kind;
    UINT32 modifiers;
    INT32  windows_key_code;
    INT32  native_key_code; /* X11 keycode */
    UINT32 is_system_key;
    UINT16 character;
    UINT16 unmodified_character;
};

/*
 * WebResourceRequested: the helper pauses a request matching a filter and sends
 * WV2_EVT_RESOURCE_REQUEST; the DLL answers with WV2_CMD_RESOURCE_RESPONSE (handled = 0 lets the
 * request continue to the network), then any number of WV2_CMD_RESOURCE_DATA chunks (streaming,
 * bounded memory) and WV2_CMD_RESOURCE_END. Headers are "Name: value\r\n" lines.
 */
struct wv2_resource_filter
{
    UINT32 context;         /* COREWEBVIEW2_WEB_RESOURCE_CONTEXT */
    UINT32 reserved;
};

struct wv2_resource_id
{
    UINT64 id;
};

struct wv2_resource_request
{
    UINT64 id;
    UINT32 context;         /* COREWEBVIEW2_WEB_RESOURCE_CONTEXT */
    UINT32 url_size;
    UINT32 method_size;
    UINT32 headers_size;
    UINT32 body_size;
    UINT32 reserved;
};

struct wv2_resource_response
{
    UINT64 id;
    UINT32 handled;
    UINT32 status;
    UINT32 reason_size;
    UINT32 headers_size;
    UINT64 content_size;
};

/* Main frame navigation, ids chosen by the helper. */
struct wv2_navigation
{
    UINT64 id;
    UINT32 a;
    UINT32 b;
};

struct wv2_navigation_completed
{
    UINT64 id;
    UINT32 success;
    UINT32 http_status;
    INT32  net_error;       /* Chromium net error code (cef_errorcode_t), 0 = none */
    UINT32 reserved;
};

struct wv2_web_message
{
    UINT32 source_size;     /* bytes of the source URL following this struct */
    UINT32 reserved;
};

struct wv2_frame_buffer
{
    UINT32 width;
    UINT32 height;
    UINT32 stride;
    UINT32 slots;
    UINT64 slot_size;
    UINT64 generation;      /* increases with every new buffer */
};

struct wv2_frame_ready
{
    UINT64 generation;
    UINT32 slot;
    UINT32 reserved;
    INT32  dirty_left, dirty_top, dirty_right, dirty_bottom;
};

struct wv2_frame_done
{
    UINT64 generation;
    UINT32 slot;
    UINT32 reserved;
};

#endif /* __WEBVIEW2_PROTOCOL_H */
