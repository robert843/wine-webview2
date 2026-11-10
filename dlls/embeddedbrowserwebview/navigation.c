/*
 * Navigation event arguments.
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

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "webview2.h"
#include "stubs.h"
#include "webview2_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(webview2);

/* One data structure behind all navigation argument objects. */
struct nav_args
{
    struct navigation_info info;
    WCHAR *uri;
};

static ICoreWebView2NavigationStartingEventArgs3Vtbl starting_vtbl;
static struct stub_family starting_family;
static ICoreWebView2ContentLoadingEventArgsVtbl content_loading_vtbl;
static struct stub_family content_loading_family;
static ICoreWebView2SourceChangedEventArgsVtbl source_changed_vtbl;
static struct stub_family source_changed_family;
static ICoreWebView2DOMContentLoadedEventArgsVtbl dom_content_loaded_vtbl;
static struct stub_family dom_content_loaded_family;
static ICoreWebView2NavigationCompletedEventArgs2Vtbl completed_vtbl;
static struct stub_family completed_family;

static struct nav_args *args_from_iface(void *iface)
{
    return ((struct stub_object *)iface)->data;
}

static void nav_args_destroy(struct stub_object *obj)
{
    struct nav_args *args = obj->data;

    free(args->uri);
    free(args);
}

/* NavigationStarting */

static HRESULT STDMETHODCALLTYPE starting_get_Uri(ICoreWebView2NavigationStartingEventArgs3 *iface, LPWSTR *uri)
{
    struct nav_args *args = args_from_iface(iface);
    size_t size;

    TRACE("iface %p, uri %p.\n", iface, uri);
    if (!uri) return E_POINTER;
    size = (wcslen(args->uri) + 1) * sizeof(WCHAR);
    if (!(*uri = CoTaskMemAlloc(size))) return E_OUTOFMEMORY;
    memcpy(*uri, args->uri, size);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE starting_get_IsUserInitiated(ICoreWebView2NavigationStartingEventArgs3 *iface, BOOL *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    if (!value) return E_POINTER;
    *value = args_from_iface(iface)->info.user_initiated;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE starting_get_IsRedirected(ICoreWebView2NavigationStartingEventArgs3 *iface, BOOL *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    if (!value) return E_POINTER;
    *value = args_from_iface(iface)->info.redirected;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE starting_get_Cancel(ICoreWebView2NavigationStartingEventArgs3 *iface, BOOL *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    if (!value) return E_POINTER;
    *value = args_from_iface(iface)->info.cancel;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE starting_put_Cancel(ICoreWebView2NavigationStartingEventArgs3 *iface, BOOL value)
{
    TRACE("iface %p, value %d.\n", iface, value);
    args_from_iface(iface)->info.cancel = !!value;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE starting_get_NavigationId(ICoreWebView2NavigationStartingEventArgs3 *iface, UINT64 *id)
{
    TRACE("iface %p, id %p.\n", iface, id);
    if (!id) return E_POINTER;
    *id = args_from_iface(iface)->info.id;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE starting_get_NavigationKind(ICoreWebView2NavigationStartingEventArgs3 *iface,
        COREWEBVIEW2_NAVIGATION_KIND *kind)
{
    TRACE("iface %p, kind %p.\n", iface, kind);
    if (!kind) return E_POINTER;
    *kind = COREWEBVIEW2_NAVIGATION_KIND_NEW_DOCUMENT;
    return S_OK;
}

/* ContentLoading */

static HRESULT STDMETHODCALLTYPE content_loading_get_IsErrorPage(ICoreWebView2ContentLoadingEventArgs *iface, BOOL *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    if (!value) return E_POINTER;
    *value = args_from_iface(iface)->info.error_page;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE content_loading_get_NavigationId(ICoreWebView2ContentLoadingEventArgs *iface, UINT64 *id)
{
    TRACE("iface %p, id %p.\n", iface, id);
    if (!id) return E_POINTER;
    *id = args_from_iface(iface)->info.id;
    return S_OK;
}

/* SourceChanged */

static HRESULT STDMETHODCALLTYPE source_changed_get_IsNewDocument(ICoreWebView2SourceChangedEventArgs *iface, BOOL *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    if (!value) return E_POINTER;
    *value = args_from_iface(iface)->info.new_document;
    return S_OK;
}

/* DOMContentLoaded */

static HRESULT STDMETHODCALLTYPE dom_content_loaded_get_NavigationId(ICoreWebView2DOMContentLoadedEventArgs *iface, UINT64 *id)
{
    TRACE("iface %p, id %p.\n", iface, id);
    if (!id) return E_POINTER;
    *id = args_from_iface(iface)->info.id;
    return S_OK;
}

/* NavigationCompleted */

static HRESULT STDMETHODCALLTYPE completed_get_IsSuccess(ICoreWebView2NavigationCompletedEventArgs2 *iface, BOOL *value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    if (!value) return E_POINTER;
    *value = args_from_iface(iface)->info.success;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE completed_get_WebErrorStatus(ICoreWebView2NavigationCompletedEventArgs2 *iface,
        COREWEBVIEW2_WEB_ERROR_STATUS *status)
{
    TRACE("iface %p, status %p.\n", iface, status);
    if (!status) return E_POINTER;
    *status = args_from_iface(iface)->info.web_error;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE completed_get_NavigationId(ICoreWebView2NavigationCompletedEventArgs2 *iface, UINT64 *id)
{
    TRACE("iface %p, id %p.\n", iface, id);
    if (!id) return E_POINTER;
    *id = args_from_iface(iface)->info.id;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE completed_get_HttpStatusCode(ICoreWebView2NavigationCompletedEventArgs2 *iface, int *status)
{
    TRACE("iface %p, status %p.\n", iface, status);
    if (!status) return E_POINTER;
    *status = args_from_iface(iface)->info.http_status;
    return S_OK;
}

IUnknown *navigation_create_args(enum navigation_args_kind kind, const struct navigation_info *info, const WCHAR *uri)
{
    static const struct stub_family *families[] =
    {
        &starting_family, &content_loading_family, &source_changed_family,
        &dom_content_loaded_family, &completed_family,
    };
    struct stub_object *obj;
    struct nav_args *args;

    if (!(args = calloc(1, sizeof(*args)))) return NULL;
    args->info = *info;
    if (!uri) uri = L"";
    if (!(args->uri = malloc((wcslen(uri) + 1) * sizeof(WCHAR))) || !(obj = stub_create(families[kind])))
    {
        free(args->uri);
        free(args);
        return NULL;
    }
    wcscpy(args->uri, uri);
    obj->data = args;
    obj->destroy = nav_args_destroy;
    return (IUnknown *)obj;
}

BOOL navigation_args_cancelled(IUnknown *iface)
{
    return args_from_iface(iface)->info.cancel;
}

/* Chromium net error -> COREWEBVIEW2_WEB_ERROR_STATUS */
COREWEBVIEW2_WEB_ERROR_STATUS navigation_web_error(int net_error)
{
    switch (net_error)
    {
    case 0: return COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN;
    case -3: return COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED;        /* ERR_ABORTED */
    case -7: case -118: return COREWEBVIEW2_WEB_ERROR_STATUS_TIMEOUT;        /* ERR_(CONNECTION_)TIMED_OUT */
    case -100: return COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED;            /* ERR_CONNECTION_CLOSED */
    case -101: return COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_RESET;
    case -102: case -104: return COREWEBVIEW2_WEB_ERROR_STATUS_CANNOT_CONNECT; /* REFUSED, FAILED */
    case -103: return COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_ABORTED;
    case -105: case -137: return COREWEBVIEW2_WEB_ERROR_STATUS_HOST_NAME_NOT_RESOLVED;
    case -106: return COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED;            /* ERR_INTERNET_DISCONNECTED */
    case -109: return COREWEBVIEW2_WEB_ERROR_STATUS_SERVER_UNREACHABLE;      /* ERR_ADDRESS_UNREACHABLE */
    case -200: return COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_COMMON_NAME_IS_INCORRECT;
    case -201: return COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_EXPIRED;
    case -202: case -207: return COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_IS_INVALID;
    case -206: return COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_REVOKED;
    case -310: return COREWEBVIEW2_WEB_ERROR_STATUS_REDIRECT_FAILED;         /* ERR_TOO_MANY_REDIRECTS */
    case -320: case -324: return COREWEBVIEW2_WEB_ERROR_STATUS_ERROR_HTTP_INVALID_SERVER_RESPONSE;
    default: return COREWEBVIEW2_WEB_ERROR_STATUS_UNEXPECTED_ERROR;
    }
}

void navigation_init(void)
{
    starting_vtbl = *(const ICoreWebView2NavigationStartingEventArgs3Vtbl *)stub_family_ICoreWebView2NavigationStartingEventArgs3.vtbl;
    starting_vtbl.get_Uri = starting_get_Uri;
    starting_vtbl.get_IsUserInitiated = starting_get_IsUserInitiated;
    starting_vtbl.get_IsRedirected = starting_get_IsRedirected;
    starting_vtbl.get_Cancel = starting_get_Cancel;
    starting_vtbl.put_Cancel = starting_put_Cancel;
    starting_vtbl.get_NavigationId = starting_get_NavigationId;
    starting_vtbl.get_NavigationKind = starting_get_NavigationKind;
    starting_family = stub_family_ICoreWebView2NavigationStartingEventArgs3;
    starting_family.vtbl = &starting_vtbl;

    content_loading_vtbl = *(const ICoreWebView2ContentLoadingEventArgsVtbl *)stub_family_ICoreWebView2ContentLoadingEventArgs.vtbl;
    content_loading_vtbl.get_IsErrorPage = content_loading_get_IsErrorPage;
    content_loading_vtbl.get_NavigationId = content_loading_get_NavigationId;
    content_loading_family = stub_family_ICoreWebView2ContentLoadingEventArgs;
    content_loading_family.vtbl = &content_loading_vtbl;

    source_changed_vtbl = *(const ICoreWebView2SourceChangedEventArgsVtbl *)stub_family_ICoreWebView2SourceChangedEventArgs.vtbl;
    source_changed_vtbl.get_IsNewDocument = source_changed_get_IsNewDocument;
    source_changed_family = stub_family_ICoreWebView2SourceChangedEventArgs;
    source_changed_family.vtbl = &source_changed_vtbl;

    dom_content_loaded_vtbl = *(const ICoreWebView2DOMContentLoadedEventArgsVtbl *)stub_family_ICoreWebView2DOMContentLoadedEventArgs.vtbl;
    dom_content_loaded_vtbl.get_NavigationId = dom_content_loaded_get_NavigationId;
    dom_content_loaded_family = stub_family_ICoreWebView2DOMContentLoadedEventArgs;
    dom_content_loaded_family.vtbl = &dom_content_loaded_vtbl;

    completed_vtbl = *(const ICoreWebView2NavigationCompletedEventArgs2Vtbl *)stub_family_ICoreWebView2NavigationCompletedEventArgs2.vtbl;
    completed_vtbl.get_IsSuccess = completed_get_IsSuccess;
    completed_vtbl.get_WebErrorStatus = completed_get_WebErrorStatus;
    completed_vtbl.get_NavigationId = completed_get_NavigationId;
    completed_vtbl.get_HttpStatusCode = completed_get_HttpStatusCode;
    completed_family = stub_family_ICoreWebView2NavigationCompletedEventArgs2;
    completed_family.vtbl = &completed_vtbl;
}
