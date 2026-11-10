/*
 * Generic COM proxy runtime (docs/architektura-proxy.md).
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

#ifndef __WEBVIEW2_PROXY_H
#define __WEBVIEW2_PROXY_H

/* Growing output buffer for call arguments / results. */
struct wv2_buffer
{
    char *data;
    size_t size, capacity;
};

/* Reader over received values; owns its data when `owned` is set. */
struct wv2_reader
{
    const char *data;
    size_t size, pos;
    char *owned;
};

void wv2_put_u32(struct wv2_buffer *buf, UINT32 value);
void wv2_put_u64(struct wv2_buffer *buf, UINT64 value);
void wv2_put_f64(struct wv2_buffer *buf, double value);
void wv2_put_bytes(struct wv2_buffer *buf, const void *data, size_t size);
void wv2_put_str(struct wv2_buffer *buf, const WCHAR *str);
void wv2_put_iface(struct wv2_buffer *buf, IUnknown *obj);
void wv2_put_stream(struct wv2_buffer *buf, IStream *stream);

UINT32 wv2_get_u32(struct wv2_reader *rd);
UINT64 wv2_get_u64(struct wv2_reader *rd);
double wv2_get_f64(struct wv2_reader *rd);
void wv2_get_bytes(struct wv2_reader *rd, void *data, size_t size);
WCHAR *wv2_get_str(struct wv2_reader *rd);      /* malloc'ed, NULL for a NULL string */
WCHAR *wv2_get_str_co(struct wv2_reader *rd);   /* CoTaskMemAlloc'ed */
IUnknown *wv2_get_iface(struct wv2_reader *rd); /* AddRef'ed proxy / application object */
IStream *wv2_get_stream(struct wv2_reader *rd); /* new memory stream or NULL */
void wv2_reader_free(struct wv2_reader *rd);

/* Proxy objects (the generated vtables call these). */
HRESULT STDMETHODCALLTYPE proxy_QueryInterface(IUnknown *iface, REFIID iid, void **out);
ULONG STDMETHODCALLTYPE proxy_AddRef(IUnknown *iface);
ULONG STDMETHODCALLTYPE proxy_Release(IUnknown *iface);
HRESULT proxy_call(IUnknown *iface, UINT32 method, struct wv2_buffer *args, struct wv2_reader *ret);
HRESULT proxy_call_root(UINT32 method, struct wv2_buffer *args, struct wv2_reader *ret);

/* Host overrides: a family can get a copy of its proxy vtable with some methods replaced by
 * hand-written code (window, painting, input...); host data is attached per proxy. */
const void *proxy_get_vtbl(UINT32 family);
void proxy_set_host_vtbl(UINT32 family, const void *vtbl);
void *proxy_get_host_data(IUnknown *iface);
void proxy_set_host_data(IUnknown *iface, void *data, void (*destroy)(void *data));
BOOL proxy_is_proxy(IUnknown *iface);

/* Messages from the reader thread. */
void proxy_handle_reply(const BYTE *payload, UINT32 size);
void proxy_handle_invoke(const BYTE *payload, UINT32 size);
void proxy_handle_release_app(const BYTE *payload, UINT32 size);
void proxy_connection_lost(void);

#endif /* __WEBVIEW2_PROXY_H */
