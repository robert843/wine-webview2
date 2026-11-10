/*
 * EmbeddedBrowserWebView unix side: helper process management and IPC.
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

#if 0
#pragma makedep unix
#endif

#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <spawn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "unixlib.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(webview2);

extern char **environ;

#define MAX_FRAME_BUFFERS 64

struct frame_buffer
{
    UINT32 view;
    UINT64 generation;
    struct wv2_frame_buffer info;
    void *base;
    size_t size;
};

struct helper
{
    int fd;
    pid_t pid;
    pthread_mutex_t lock;   /* protects buffers */
    struct frame_buffer buffers[MAX_FRAME_BUFFERS];
    char msg[WV2_MAX_MESSAGE];  /* receive buffer, used only by the reader thread */
};

static struct helper *get_helper(helper_handle_t handle)
{
    return (struct helper *)(ULONG_PTR)handle;
}

static const char *find_helper_path(char *buf, size_t size)
{
    const char *env = getenv("WINE_WEBVIEW2_HELPER");
    Dl_info info;
    char *p;

    if (env && *env) return env;
    if (dladdr((void *)find_helper_path, &info) && info.dli_fname)
    {
        snprintf(buf, size, "%s", info.dli_fname);
        if ((p = strrchr(buf, '/')))
        {
            snprintf(p + 1, size - (p + 1 - buf), "wine-webview2-helper");
            return buf;
        }
    }
    return "/usr/lib/wine-webview2/wine-webview2-helper";
}

static NTSTATUS send_message(struct helper *helper, UINT32 type, UINT32 view, const void *data, UINT32 size)
{
    struct wv2_msg_header header = {type, sizeof(header) + size, view, 0};
    struct iovec iov[2] = {{&header, sizeof(header)}, {(void *)data, size}};
    struct msghdr msg = {0};
    ssize_t ret;

    if (header.size > WV2_MAX_MESSAGE) return STATUS_BUFFER_OVERFLOW;
    msg.msg_iov = iov;
    msg.msg_iovlen = size ? 2 : 1;
    do ret = sendmsg(helper->fd, &msg, MSG_NOSIGNAL);
    while (ret < 0 && errno == EINTR);
    if (ret < 0)
    {
        WARN("sendmsg failed: %s\n", strerror(errno));
        return STATUS_PIPE_BROKEN;
    }
    return STATUS_SUCCESS;
}

/* Receives one message into helper->msg; returns the received descriptor (or -1) in *fd. */
static NTSTATUS recv_message(struct helper *helper, struct wv2_msg_header **header, int *fd)
{
    char control[CMSG_SPACE(sizeof(int))];
    struct iovec iov = {helper->msg, sizeof(helper->msg)};
    struct msghdr msg = {0};
    struct cmsghdr *cmsg;
    ssize_t ret;

    *fd = -1;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);
    do ret = recvmsg(helper->fd, &msg, MSG_CMSG_CLOEXEC);
    while (ret < 0 && errno == EINTR);
    if (ret <= 0)
    {
        if (ret < 0) WARN("recvmsg failed: %s\n", strerror(errno));
        else TRACE("helper closed the connection\n");
        return STATUS_PIPE_BROKEN;
    }
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg))
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
            memcpy(fd, CMSG_DATA(cmsg), sizeof(int));

    *header = (struct wv2_msg_header *)helper->msg;
    if (ret < sizeof(**header) || (*header)->size != ret || (msg.msg_flags & MSG_TRUNC))
    {
        ERR("malformed message (%zd bytes, flags %#x)\n", ret, msg.msg_flags);
        if (*fd != -1) close(*fd);
        return STATUS_INVALID_NETWORK_RESPONSE;
    }
    return STATUS_SUCCESS;
}

static void install_frame_buffer(struct helper *helper, UINT32 view, const struct wv2_frame_buffer *info, int fd)
{
    struct frame_buffer *slot = NULL;
    size_t size = info->slot_size * info->slots;
    void *base;
    unsigned int i;

    if (fd == -1 || !size || info->stride < info->width * 4
            || info->slot_size < (UINT64)info->stride * info->height)
    {
        ERR("invalid frame buffer for view %u\n", view);
        if (fd != -1) close(fd);
        return;
    }
    base = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (base == MAP_FAILED)
    {
        ERR("mmap of frame buffer failed: %s\n", strerror(errno));
        return;
    }

    pthread_mutex_lock(&helper->lock);
    for (i = 0; i < MAX_FRAME_BUFFERS; i++)
    {
        if (helper->buffers[i].base && helper->buffers[i].view == view)
        {
            munmap(helper->buffers[i].base, helper->buffers[i].size);
            helper->buffers[i].base = NULL;
        }
        if (!helper->buffers[i].base && !slot) slot = &helper->buffers[i];
    }
    if (slot)
    {
        slot->view = view;
        slot->generation = info->generation;
        slot->info = *info;
        slot->base = base;
        slot->size = size;
    }
    else munmap(base, size);
    pthread_mutex_unlock(&helper->lock);
    TRACE("view %u: frame buffer %ux%u stride %u, generation %s\n", view, info->width, info->height,
          info->stride, wine_dbgstr_longlong(info->generation));
}

static void close_helper(struct helper *helper)
{
    unsigned int i;

    if (helper->fd != -1) close(helper->fd);
    if (helper->pid > 0) waitpid(helper->pid, NULL, 0);
    for (i = 0; i < MAX_FRAME_BUFFERS; i++)
        if (helper->buffers[i].base) munmap(helper->buffers[i].base, helper->buffers[i].size);
    pthread_mutex_destroy(&helper->lock);
    free(helper);
}

static NTSTATUS helper_start(void *args)
{
    struct helper_start_params *params = args;
    posix_spawn_file_actions_t actions;
    struct wv2_msg_header *header;
    struct wv2_hello hello = {WV2_PROTOCOL_VERSION, getpid()};
    char path_buf[PATH_MAX], fd_arg[32];
    const char *path = find_helper_path(path_buf, sizeof(path_buf));
    char *argv[] = {(char *)path, fd_arg, NULL};
    struct helper *helper;
    int fds[2], fd, ret;
    NTSTATUS status;

    if (!(helper = calloc(1, sizeof(*helper)))) return STATUS_NO_MEMORY;
    if (socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, fds) < 0)
    {
        ERR("socketpair failed: %s\n", strerror(errno));
        free(helper);
        return STATUS_UNSUCCESSFUL;
    }
    helper->fd = fds[0];
    pthread_mutex_init(&helper->lock, NULL);

    /* fd 3 in the helper; dup2 clears FD_CLOEXEC on the target. */
    snprintf(fd_arg, sizeof(fd_arg), "--ipc-fd=3");
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, fds[1], 3);
    TRACE("starting helper %s\n", debugstr_a(path));
    ret = posix_spawn(&helper->pid, path, &actions, NULL, argv, environ);
    posix_spawn_file_actions_destroy(&actions);
    close(fds[1]);
    if (ret)
    {
        ERR("cannot start helper %s: %s\n", debugstr_a(path), strerror(ret));
        helper->pid = 0;
        close_helper(helper);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if ((status = send_message(helper, WV2_CMD_HELLO, 0, &hello, sizeof(hello))) ||
        (status = recv_message(helper, &header, &fd)))
    {
        ERR("handshake with helper failed: %#x\n", (int)status);
        close_helper(helper);
        return status;
    }
    if (fd != -1) close(fd);
    if (header->type != WV2_EVT_HELLO || header->size < sizeof(*header) + sizeof(hello) ||
        ((struct wv2_hello *)(header + 1))->version != WV2_PROTOCOL_VERSION)
    {
        ERR("helper protocol mismatch (type %#x)\n", header->type);
        close_helper(helper);
        return STATUS_REVISION_MISMATCH;
    }

    params->handle = (ULONG_PTR)helper;
    params->helper_pid = helper->pid;
    params->helper_version = ((struct wv2_hello *)(header + 1))->version;
    TRACE("helper pid %d ready, protocol %u\n", (int)helper->pid, params->helper_version);
    return STATUS_SUCCESS;
}

static NTSTATUS helper_stop(void *args)
{
    struct helper_stop_params *params = args;
    struct helper *helper = get_helper(params->handle);

    send_message(helper, WV2_CMD_SHUTDOWN, 0, NULL, 0);
    shutdown(helper->fd, SHUT_WR);
    close_helper(helper);
    return STATUS_SUCCESS;
}

static NTSTATUS helper_send(void *args)
{
    struct helper_send_params *params = args;

    return send_message(get_helper(params->handle), params->type, params->view, params->data, params->size);
}

static NTSTATUS helper_recv(void *args)
{
    struct helper_recv_params *params = args;
    struct helper *helper = get_helper(params->handle);
    struct wv2_msg_header *header;
    UINT32 payload;
    NTSTATUS status;
    int fd;

    if ((status = recv_message(helper, &header, &fd))) return status;

    payload = header->size - sizeof(*header);
    if (header->type == WV2_EVT_FRAME_BUFFER && payload >= sizeof(struct wv2_frame_buffer))
        install_frame_buffer(helper, header->view, (struct wv2_frame_buffer *)(header + 1), fd);
    else if (fd != -1)
        close(fd);

    params->header = *header;
    memcpy(params->buffer, header + 1, min(payload, params->size));
    return STATUS_SUCCESS;
}

static NTSTATUS helper_copy_frame(void *args)
{
    struct helper_copy_frame_params *params = args;
    struct helper *helper = get_helper(params->handle);
    struct wv2_frame_done done = {params->generation, params->slot, 0};
    NTSTATUS status = STATUS_NOT_FOUND;
    unsigned int i, y, rows, bytes;

    pthread_mutex_lock(&helper->lock);
    for (i = 0; i < MAX_FRAME_BUFFERS; i++)
    {
        struct frame_buffer *buf = &helper->buffers[i];
        const char *src;

        if (!buf->base || buf->view != params->view || buf->generation != params->generation) continue;
        if (params->slot >= buf->info.slots) break;
        src = (const char *)buf->base + buf->info.slot_size * params->slot;
        rows = min(params->height, buf->info.height);
        bytes = min(params->width, buf->info.width) * 4;
        for (y = 0; y < rows; y++)
            memcpy((char *)params->dst + (size_t)y * params->dst_stride, src + (size_t)y * buf->info.stride, bytes);
        status = STATUS_SUCCESS;
        break;
    }
    pthread_mutex_unlock(&helper->lock);

    send_message(helper, WV2_CMD_FRAME_DONE, params->view, &done, sizeof(done));
    return status;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    helper_start,
    helper_stop,
    helper_send,
    helper_recv,
    helper_copy_frame,
};

C_ASSERT(ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count);
