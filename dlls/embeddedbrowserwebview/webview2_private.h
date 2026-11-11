/*
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

#ifndef __WEBVIEW2_PRIVATE_H
#define __WEBVIEW2_PRIVATE_H

/* Version reported to applications. The third component must not be lower than the SDK
 * build an application was compiled against, otherwise the loader rejects the runtime. */
/* Major = Chromium version of the bundled CEF, so the user agent and the runtime version agree. */
#define WEBVIEW2_VERSION_MAJOR 156
#define WEBVIEW2_VERSION_MINOR 0
#define WEBVIEW2_VERSION_BUILD 9999
#define WEBVIEW2_VERSION_PATCH 0
#define WEBVIEW2_VERSION_STR   "156.0.9999.0"
#define WEBVIEW2_VERSION_WSTR  L"156.0.9999.0"
#define WEBVIEW2_VERSION_MAJOR_WSTR L"156"

#ifndef RC_INVOKED

extern HINSTANCE webview2_instance;

/* Runs func(arg) later on the calling thread through its message loop (like WebView2 delivers
 * completion handlers). Returns FALSE if the call could not be queued. */
BOOL webview2_queue_async(void (*func)(void *arg), void *arg);
/* Same, but on the thread owning async window hwnd (see webview2_get_async_window). */
BOOL webview2_queue_async_to(HWND hwnd, void (*func)(void *arg), void *arg);
/* Per-thread message-only window used for asynchronous delivery. */
HWND webview2_get_async_window(void);

/* controller.c */
void controller_init(void);
BOOL helper_ensure_started(const WCHAR *user_data_dir);
HRESULT controller_create(IUnknown *environment, HWND parent, IUnknown *options,
                          ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *handler);
/* hostobject.c */
struct host_objects;
struct wv2_buffer;
HRESULT host_objects_add(struct host_objects **objects, const WCHAR *name, VARIANT *object);
HRESULT host_objects_remove(struct host_objects *objects, const WCHAR *name);
void host_objects_free(struct host_objects *objects);
void host_objects_call(struct host_objects *objects, const void *payload, UINT32 size, struct wv2_buffer *reply);

/* Sends a protocol message (any thread); view 0 for generic calls. */
HRESULT webview2_send(UINT32 view, UINT32 type, const void *data, UINT32 size);

#endif /* RC_INVOKED */

#endif /* __WEBVIEW2_PRIVATE_H */
