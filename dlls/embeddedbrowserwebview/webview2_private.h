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
#define WEBVIEW2_VERSION_MAJOR 143
#define WEBVIEW2_VERSION_MINOR 0
#define WEBVIEW2_VERSION_BUILD 9999
#define WEBVIEW2_VERSION_PATCH 0
#define WEBVIEW2_VERSION_STR   "143.0.9999.0"
#define WEBVIEW2_VERSION_WSTR  L"143.0.9999.0"

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
void helper_ensure_started(void);
HRESULT controller_create(HWND parent, ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *handler);
/* Sends a protocol command for a view (any thread). */
HRESULT webview2_send(UINT32 view, UINT32 type, const void *data, UINT32 size);

/* resource.c */
void resource_init(void);
HRESULT resource_create_response(IStream *content, int status, LPCWSTR reason, LPCWSTR headers,
                                 ICoreWebView2WebResourceResponse **response);
void resource_dispatch_request(UINT32 view, ICoreWebView2 *webview, IUnknown **handlers, unsigned int count,
                               const BYTE *payload, UINT32 size);

#endif /* RC_INVOKED */

#endif /* __WEBVIEW2_PRIVATE_H */
