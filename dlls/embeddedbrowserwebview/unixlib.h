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

#ifndef __WEBVIEW2_UNIXLIB_H
#define __WEBVIEW2_UNIXLIB_H

#include "wine/unixlib.h"
#include "protocol.h"

/* Opaque handle of a running helper process (unix side pointer). */
typedef UINT64 helper_handle_t;

struct helper_start_params
{
    helper_handle_t handle;     /* out */
    UINT32 helper_pid;          /* out */
    UINT32 helper_version;      /* out */
};

struct helper_stop_params
{
    helper_handle_t handle;
};

struct helper_send_params
{
    helper_handle_t handle;
    UINT32 type;
    UINT32 view;
    const void *data;
    UINT32 size;
};

/* Blocks until the next message from the helper. FRAME_BUFFER descriptors are mapped on the
 * unix side; the payload is copied to the caller's buffer (truncated to size). */
struct helper_recv_params
{
    helper_handle_t handle;
    void *buffer;
    UINT32 size;
    struct wv2_msg_header header;   /* out */
};

/* Copies a ready frame slot into a top-down BGRA buffer and returns the slot to the helper. */
struct helper_copy_frame_params
{
    helper_handle_t handle;
    UINT32 view;
    UINT32 slot;
    UINT64 generation;
    void *dst;
    UINT32 dst_stride;
    UINT32 width;
    UINT32 height;
};

enum unix_funcs
{
    unix_helper_start,
    unix_helper_stop,
    unix_helper_send,
    unix_helper_recv,
    unix_helper_copy_frame,
    unix_funcs_count,
};

#endif /* __WEBVIEW2_UNIXLIB_H */
