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

/* Runs func(arg) later on the calling thread through its message loop (like WebView2 delivers
 * completion handlers). Returns FALSE if the call could not be queued. */
BOOL webview2_queue_async(void (*func)(void *arg), void *arg);

#endif /* __WEBVIEW2_PRIVATE_H */
