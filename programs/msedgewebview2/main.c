/*
 * msedgewebview2.exe placeholder
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

/* The browser process of the real runtime. Ours lives in the native helper, but applications
 * (e.g. LeviLauncher) check that this file exists in the runtime folder, so we ship a stub. */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(webview2);

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev, WCHAR *cmdline, int show)
{
    FIXME("msedgewebview2.exe started with %s; the browser runs in the native helper\n", debugstr_w(cmdline));
    return 0;
}
