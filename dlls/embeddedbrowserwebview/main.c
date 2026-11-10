/*
 * EmbeddedBrowserWebView — open implementation of the WebView2 runtime client DLL
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
#include "winuser.h"
#include "objbase.h"
#include "initguid.h"
#include "webview2.h"
#include "stubs.h"
#include "webview2_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(webview2);

HINSTANCE webview2_instance;

/*
 * Asynchronous callbacks.
 *
 * WebView2 never invokes completion handlers from inside the call that takes them; they are
 * delivered later on the calling (UI) thread through its message loop. We do the same with a
 * per-thread message-only window.
 */

#define WM_WEBVIEW2_ASYNC (WM_APP + 0x100)
static const WCHAR async_class_name[] = L"WineWebView2AsyncWindow";
static DWORD async_window_tls = TLS_OUT_OF_INDEXES;

struct async_call
{
    void (*func)(void *arg);
    void *arg;
};

static LRESULT CALLBACK async_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_WEBVIEW2_ASYNC)
    {
        struct async_call *call = (struct async_call *)lparam;
        call->func(call->arg);
        free(call);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

HWND webview2_get_async_window(void)
{
    HWND hwnd = TlsGetValue(async_window_tls);

    if (!hwnd)
    {
        hwnd = CreateWindowExW(0, async_class_name, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE,
                               NULL, webview2_instance, NULL);
        if (!hwnd) ERR("Failed to create async window, error %lu.\n", GetLastError());
        TlsSetValue(async_window_tls, hwnd);
    }
    return hwnd;
}

BOOL webview2_queue_async_to(HWND hwnd, void (*func)(void *arg), void *arg)
{
    struct async_call *call;

    if (!hwnd) return FALSE;
    if (!(call = malloc(sizeof(*call)))) return FALSE;
    call->func = func;
    call->arg = arg;
    if (!PostMessageW(hwnd, WM_WEBVIEW2_ASYNC, 0, (LPARAM)call))
    {
        free(call);
        return FALSE;
    }
    return TRUE;
}

BOOL webview2_queue_async(void (*func)(void *arg), void *arg)
{
    return webview2_queue_async_to(webview2_get_async_window(), func, arg);
}

/*
 * Hand-written overrides of generated stubs. During the skeleton phase only the calls needed
 * to get an application from environment creation to a usable ICoreWebView2 are implemented;
 * everything else is logged by the generated stubs.
 */

static ICoreWebView2Environment15Vtbl environment_vtbl;
static struct stub_family environment_family;
static HRESULT STDMETHODCALLTYPE environment_CreateCoreWebView2Controller(ICoreWebView2Environment15 *iface,
        HWND parent, ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *handler)
{
    TRACE("iface %p, parent %p, handler %p.\n", iface, parent, handler);
    return controller_create(parent, handler);
}

static HRESULT STDMETHODCALLTYPE environment_CreateCoreWebView2ControllerWithOptions(ICoreWebView2Environment15 *iface,
        HWND parent, ICoreWebView2ControllerOptions *options,
        ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *handler)
{
    FIXME("iface %p, parent %p, options %p, handler %p, options ignored.\n", iface, parent, options, handler);
    return controller_create(parent, handler);
}

static HRESULT STDMETHODCALLTYPE environment_get_BrowserVersionString(ICoreWebView2Environment15 *iface,
        LPWSTR *version)
{
    TRACE("iface %p, version %p.\n", iface, version);
    if (!version) return E_POINTER;
    if (!(*version = CoTaskMemAlloc(sizeof(WEBVIEW2_VERSION_WSTR)))) return E_OUTOFMEMORY;
    memcpy(*version, WEBVIEW2_VERSION_WSTR, sizeof(WEBVIEW2_VERSION_WSTR));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE environment_CreateWebResourceResponse(ICoreWebView2Environment15 *iface,
        IStream *content, int status, LPCWSTR reason, LPCWSTR headers, ICoreWebView2WebResourceResponse **response)
{
    TRACE("iface %p, content %p, status %d, reason %s, headers %s, response %p.\n", iface, content, status,
          debugstr_w(reason), debugstr_w(headers), response);
    return resource_create_response(content, status, reason, headers, response);
}

static void init_overrides(void)
{
    environment_vtbl = *(const ICoreWebView2Environment15Vtbl *)stub_family_ICoreWebView2Environment15.vtbl;
    environment_vtbl.CreateCoreWebView2Controller = environment_CreateCoreWebView2Controller;
    environment_vtbl.CreateCoreWebView2ControllerWithOptions = environment_CreateCoreWebView2ControllerWithOptions;
    environment_vtbl.get_BrowserVersionString = environment_get_BrowserVersionString;
    environment_vtbl.CreateWebResourceResponse = environment_CreateWebResourceResponse;
    environment_family = stub_family_ICoreWebView2Environment15;
    environment_family.vtbl = &environment_vtbl;

    controller_init();
    resource_init();
    navigation_init();
    settings_init();
}

/*
 * Environment creation — the entry point used by WebView2Loader.
 */

struct environment_created_ctx
{
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *handler;
    ICoreWebView2Environment *environment;
};

static void environment_created_cb(void *arg)
{
    struct environment_created_ctx *ctx = arg;
    HRESULT hr;

    TRACE("invoking environment completed handler %p, environment %p\n", ctx->handler, ctx->environment);
    hr = ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler_Invoke(ctx->handler, S_OK, ctx->environment);
    TRACE("handler returned %#lx\n", hr);
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler_Release(ctx->handler);
    ICoreWebView2Environment_Release(ctx->environment);
    free(ctx);
}

static void trace_options(IUnknown *options)
{
    ICoreWebView2EnvironmentOptions *opts;
    WCHAR *str;

    if (!options || !TRACE_ON(webview2)) return;
    if (FAILED(IUnknown_QueryInterface(options, &IID_ICoreWebView2EnvironmentOptions, (void **)&opts)))
    {
        TRACE("options %p do not implement ICoreWebView2EnvironmentOptions\n", options);
        return;
    }
    if (SUCCEEDED(ICoreWebView2EnvironmentOptions_get_AdditionalBrowserArguments(opts, &str)))
    {
        TRACE("additional browser arguments %s\n", debugstr_w(str));
        CoTaskMemFree(str);
    }
    if (SUCCEEDED(ICoreWebView2EnvironmentOptions_get_Language(opts, &str)))
    {
        TRACE("language %s\n", debugstr_w(str));
        CoTaskMemFree(str);
    }
    if (SUCCEEDED(ICoreWebView2EnvironmentOptions_get_TargetCompatibleBrowserVersion(opts, &str)))
    {
        TRACE("target compatible browser version %s\n", debugstr_w(str));
        CoTaskMemFree(str);
    }
    ICoreWebView2EnvironmentOptions_Release(opts);
}

/* The loader passes a one-byte bool as the first argument; the upper bits of the register are
 * garbage, hence BOOLEAN and not BOOL. */
HRESULT WINAPI CreateWebViewEnvironmentWithOptionsInternal(BOOLEAN check_running_instance,
        UINT32 runtime_type, const WCHAR *user_data_dir, IUnknown *options,
        ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *handler)
{
    struct environment_created_ctx *ctx;
    HMODULE module;

    TRACE("check_running_instance %d, runtime_type %u, user_data_dir %s, options %p, handler %p.\n",
          check_running_instance, runtime_type, debugstr_w(user_data_dir), options, handler);

    if (!handler) return E_POINTER;
    trace_options(options);

    /* The loader calls FreeLibrary() on us right after this call when DllCanUnloadNow is
     * exported; objects we hand out must keep the module alive. Pin it for now. */
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                       (const WCHAR *)CreateWebViewEnvironmentWithOptionsInternal, &module);

    helper_ensure_started();

    if (!(ctx = calloc(1, sizeof(*ctx)))) return E_OUTOFMEMORY;
    if (!(ctx->environment = stub_create(&environment_family)))
    {
        free(ctx);
        return E_OUTOFMEMORY;
    }
    ctx->handler = handler;
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler_AddRef(handler);
    if (!webview2_queue_async(environment_created_cb, ctx))
    {
        ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler_Release(handler);
        ICoreWebView2Environment_Release(ctx->environment);
        free(ctx);
        return E_FAIL;
    }
    return S_OK;
}

HRESULT WINAPI CreateSharedWebViewEnvironmentInternal(void)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    WNDCLASSW cls = {0};

    TRACE("instance %p, reason %lu, reserved %p.\n", instance, reason, reserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        webview2_instance = instance;
        DisableThreadLibraryCalls(instance);
        async_window_tls = TlsAlloc();
        cls.lpfnWndProc = async_wndproc;
        cls.hInstance = instance;
        cls.lpszClassName = async_class_name;
        RegisterClassW(&cls);
        init_overrides();
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        UnregisterClassW(async_class_name, instance);
        TlsFree(async_window_tls);
        break;
    }
    return TRUE;
}
