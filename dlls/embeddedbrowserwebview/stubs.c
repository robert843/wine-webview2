/*
 * Szkielet logujący WebView2 — wygenerowany przez tools/gen-stubs, NIE EDYTOWAĆ ręcznie.
 * Rodzin: 94, metod (z IUnknown): 1152.
 *
 * Każda metoda loguje FIXME i zwraca S_OK z wyzerowanymi parametrami wyjściowymi.
 */

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "webview2.h"
#include "stubs.h"
#include "webview2_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(webview2);

static void stub_invoke_async(void (*func)(void *arg), IUnknown *handler)
{
    handler->lpVtbl->AddRef(handler);
    if (!webview2_queue_async(func, handler)) handler->lpVtbl->Release(handler);
}

static HRESULT STDMETHODCALLTYPE stub_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct stub_object *obj = (struct stub_object *)iface;
    const IID *const *p;

    for (p = obj->family->iids; *p; p++)
    {
        if (IsEqualGUID(iid, *p))
        {
            InterlockedIncrement(&obj->ref);
            *out = iface;
            return S_OK;
        }
    }
    FIXME("%s %p: unsupported interface %s\n", obj->family->name, iface, debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE stub_AddRef(IUnknown *iface)
{
    struct stub_object *obj = (struct stub_object *)iface;
    ULONG ref = InterlockedIncrement(&obj->ref);
    TRACE("%s %p increasing refcount to %lu.\n", obj->family->name, iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE stub_Release(IUnknown *iface)
{
    struct stub_object *obj = (struct stub_object *)iface;
    ULONG ref = InterlockedDecrement(&obj->ref);
    TRACE("%s %p decreasing refcount to %lu.\n", obj->family->name, iface, ref);
    if (!ref)
    {
        if (obj->destroy) obj->destroy(obj);
        free(obj);
    }
    return ref;
}

void *stub_create(const struct stub_family *family)
{
    struct stub_object *obj;

    if (!(obj = calloc(1, sizeof(*obj)))) return NULL;
    obj->vtbl = family->vtbl;
    obj->ref = 1;
    obj->family = family;
    FIXME("created %s stub %p\n", family->name, obj);
    return obj;
}

static INT64 stub_next_token(void)
{
    static LONG64 next;
    return InterlockedIncrement64(&next);
}

static WCHAR *stub_strdup(const WCHAR *str)
{
    size_t size = (wcslen(str) + 1) * sizeof(WCHAR);
    WCHAR *ret = CoTaskMemAlloc(size);
    if (ret) memcpy(ret, str, size);
    return ret;
}

/* ===== wywołania handlerów zakończenia ===== */

static void invoke_ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler(void *arg)
{
    ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, L"");
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2BrowserExtensionEnableCompletedHandler(void *arg)
{
    ICoreWebView2BrowserExtensionEnableCompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2BrowserExtensionEnableCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2BrowserExtensionRemoveCompletedHandler(void *arg)
{
    ICoreWebView2BrowserExtensionRemoveCompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2BrowserExtensionRemoveCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2CallDevToolsProtocolMethodCompletedHandler(void *arg)
{
    ICoreWebView2CallDevToolsProtocolMethodCompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2CallDevToolsProtocolMethodCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, L"");
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2CapturePreviewCompletedHandler(void *arg)
{
    ICoreWebView2CapturePreviewCompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2CapturePreviewCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2ClearBrowsingDataCompletedHandler(void *arg)
{
    ICoreWebView2ClearBrowsingDataCompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2ClearBrowsingDataCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2ClearServerCertificateErrorActionsCompletedHandler(void *arg)
{
    ICoreWebView2ClearServerCertificateErrorActionsCompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2ClearServerCertificateErrorActionsCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler(void *arg)
{
    ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler *handler = arg;
    ICoreWebView2CompositionController *arg1 = stub_create(&stub_family_ICoreWebView2CompositionController5);

    TRACE("invoking ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, arg1);
    if (arg1) arg1->lpVtbl->Release(arg1);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler(void *arg)
{
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *handler = arg;
    ICoreWebView2Controller *arg1 = stub_create(&stub_family_ICoreWebView2Controller4);

    TRACE("invoking ICoreWebView2CreateCoreWebView2ControllerCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, arg1);
    if (arg1) arg1->lpVtbl->Release(arg1);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2ExecuteScriptCompletedHandler(void *arg)
{
    ICoreWebView2ExecuteScriptCompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2ExecuteScriptCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, L"");
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2ExecuteScriptWithResultCompletedHandler(void *arg)
{
    ICoreWebView2ExecuteScriptWithResultCompletedHandler *handler = arg;
    ICoreWebView2ExecuteScriptResult *arg1 = stub_create(&stub_family_ICoreWebView2ExecuteScriptResult);

    TRACE("invoking ICoreWebView2ExecuteScriptWithResultCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, arg1);
    if (arg1) arg1->lpVtbl->Release(arg1);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2FindStartCompletedHandler(void *arg)
{
    ICoreWebView2FindStartCompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2FindStartCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2GetCookiesCompletedHandler(void *arg)
{
    ICoreWebView2GetCookiesCompletedHandler *handler = arg;
    ICoreWebView2CookieList *arg1 = stub_create(&stub_family_ICoreWebView2CookieList);

    TRACE("invoking ICoreWebView2GetCookiesCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, arg1);
    if (arg1) arg1->lpVtbl->Release(arg1);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2GetFaviconCompletedHandler(void *arg)
{
    ICoreWebView2GetFaviconCompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2GetFaviconCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, NULL);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2GetNonDefaultPermissionSettingsCompletedHandler(void *arg)
{
    ICoreWebView2GetNonDefaultPermissionSettingsCompletedHandler *handler = arg;
    ICoreWebView2PermissionSettingCollectionView *arg1 = stub_create(&stub_family_ICoreWebView2PermissionSettingCollectionView);

    TRACE("invoking ICoreWebView2GetNonDefaultPermissionSettingsCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, arg1);
    if (arg1) arg1->lpVtbl->Release(arg1);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2GetProcessExtendedInfosCompletedHandler(void *arg)
{
    ICoreWebView2GetProcessExtendedInfosCompletedHandler *handler = arg;
    ICoreWebView2ProcessExtendedInfoCollection *arg1 = stub_create(&stub_family_ICoreWebView2ProcessExtendedInfoCollection);

    TRACE("invoking ICoreWebView2GetProcessExtendedInfosCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, arg1);
    if (arg1) arg1->lpVtbl->Release(arg1);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2GetServiceWorkerRegistrationsCompletedHandler(void *arg)
{
    ICoreWebView2GetServiceWorkerRegistrationsCompletedHandler *handler = arg;
    ICoreWebView2ServiceWorkerRegistrationCollectionView *arg1 = stub_create(&stub_family_ICoreWebView2ServiceWorkerRegistrationCollectionView);

    TRACE("invoking ICoreWebView2GetServiceWorkerRegistrationsCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, arg1);
    if (arg1) arg1->lpVtbl->Release(arg1);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2GetSharedWorkersCompletedHandler(void *arg)
{
    ICoreWebView2GetSharedWorkersCompletedHandler *handler = arg;
    ICoreWebView2SharedWorkerCollectionView *arg1 = stub_create(&stub_family_ICoreWebView2SharedWorkerCollectionView);

    TRACE("invoking ICoreWebView2GetSharedWorkersCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, arg1);
    if (arg1) arg1->lpVtbl->Release(arg1);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2PrintCompletedHandler(void *arg)
{
    ICoreWebView2PrintCompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2PrintCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, 0);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2PrintToPdfCompletedHandler(void *arg)
{
    ICoreWebView2PrintToPdfCompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2PrintToPdfCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, 0);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2PrintToPdfStreamCompletedHandler(void *arg)
{
    ICoreWebView2PrintToPdfStreamCompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2PrintToPdfStreamCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, NULL);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2ProfileAddBrowserExtensionCompletedHandler(void *arg)
{
    ICoreWebView2ProfileAddBrowserExtensionCompletedHandler *handler = arg;
    ICoreWebView2BrowserExtension *arg1 = stub_create(&stub_family_ICoreWebView2BrowserExtension);

    TRACE("invoking ICoreWebView2ProfileAddBrowserExtensionCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, arg1);
    if (arg1) arg1->lpVtbl->Release(arg1);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2ProfileGetBrowserExtensionsCompletedHandler(void *arg)
{
    ICoreWebView2ProfileGetBrowserExtensionsCompletedHandler *handler = arg;
    ICoreWebView2BrowserExtensionList *arg1 = stub_create(&stub_family_ICoreWebView2BrowserExtensionList);

    TRACE("invoking ICoreWebView2ProfileGetBrowserExtensionsCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, arg1);
    if (arg1) arg1->lpVtbl->Release(arg1);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2SetPermissionStateCompletedHandler(void *arg)
{
    ICoreWebView2SetPermissionStateCompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2SetPermissionStateCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2ShowSaveAsUICompletedHandler(void *arg)
{
    ICoreWebView2ShowSaveAsUICompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2ShowSaveAsUICompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, 0);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2TrySuspendCompletedHandler(void *arg)
{
    ICoreWebView2TrySuspendCompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2TrySuspendCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, 0);
    handler->lpVtbl->Release(handler);
}

static void invoke_ICoreWebView2WebResourceResponseViewGetContentCompletedHandler(void *arg)
{
    ICoreWebView2WebResourceResponseViewGetContentCompletedHandler *handler = arg;

    TRACE("invoking ICoreWebView2WebResourceResponseViewGetContentCompletedHandler %p\n", handler);
    handler->lpVtbl->Invoke(handler, S_OK, NULL);
    handler->lpVtbl->Release(handler);
}

/* ===== ICoreWebView2RegionRectCollectionView ===== */

static const IID *const ICoreWebView2RegionRectCollectionView_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2RegionRectCollectionView,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2RegionRectCollectionView_get_Count(ICoreWebView2RegionRectCollectionView *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2RegionRectCollectionView_GetValueAtIndex(ICoreWebView2RegionRectCollectionView *iface, UINT32 index, RECT *value)
{
    FIXME("iface %p, index %d, value %p stub!\n", iface, (int)index, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2RegionRectCollectionViewVtbl ICoreWebView2RegionRectCollectionView_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2RegionRectCollectionView_get_Count,
    ICoreWebView2RegionRectCollectionView_GetValueAtIndex,
};

const struct stub_family stub_family_ICoreWebView2RegionRectCollectionView =
{
    "ICoreWebView2RegionRectCollectionView",
    &ICoreWebView2RegionRectCollectionView_stub_vtbl,
    ICoreWebView2RegionRectCollectionView_iids,
};

/* ===== ICoreWebView2StringCollection ===== */

static const IID *const ICoreWebView2StringCollection_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2StringCollection,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2StringCollection_get_Count(ICoreWebView2StringCollection *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2StringCollection_GetValueAtIndex(ICoreWebView2StringCollection *iface, UINT32 index, LPWSTR *value)
{
    FIXME("iface %p, index %d, value %p stub!\n", iface, (int)index, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static const ICoreWebView2StringCollectionVtbl ICoreWebView2StringCollection_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2StringCollection_get_Count,
    ICoreWebView2StringCollection_GetValueAtIndex,
};

const struct stub_family stub_family_ICoreWebView2StringCollection =
{
    "ICoreWebView2StringCollection",
    &ICoreWebView2StringCollection_stub_vtbl,
    ICoreWebView2StringCollection_iids,
};

/* ===== ICoreWebView2ClientCertificateCollection ===== */

static const IID *const ICoreWebView2ClientCertificateCollection_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ClientCertificateCollection,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificateCollection_get_Count(ICoreWebView2ClientCertificateCollection *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificateCollection_GetValueAtIndex(ICoreWebView2ClientCertificateCollection *iface, UINT32 index, ICoreWebView2ClientCertificate **value)
{
    FIXME("iface %p, index %d, value %p stub!\n", iface, (int)index, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ClientCertificate);
    return S_OK;
}

static const ICoreWebView2ClientCertificateCollectionVtbl ICoreWebView2ClientCertificateCollection_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ClientCertificateCollection_get_Count,
    ICoreWebView2ClientCertificateCollection_GetValueAtIndex,
};

const struct stub_family stub_family_ICoreWebView2ClientCertificateCollection =
{
    "ICoreWebView2ClientCertificateCollection",
    &ICoreWebView2ClientCertificateCollection_stub_vtbl,
    ICoreWebView2ClientCertificateCollection_iids,
};

/* ===== ICoreWebView2CookieList ===== */

static const IID *const ICoreWebView2CookieList_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2CookieList,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2CookieList_get_Count(ICoreWebView2CookieList *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CookieList_GetValueAtIndex(ICoreWebView2CookieList *iface, UINT32 index, ICoreWebView2Cookie **value)
{
    FIXME("iface %p, index %d, value %p stub!\n", iface, (int)index, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2Cookie);
    return S_OK;
}

static const ICoreWebView2CookieListVtbl ICoreWebView2CookieList_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2CookieList_get_Count,
    ICoreWebView2CookieList_GetValueAtIndex,
};

const struct stub_family stub_family_ICoreWebView2CookieList =
{
    "ICoreWebView2CookieList",
    &ICoreWebView2CookieList_stub_vtbl,
    ICoreWebView2CookieList_iids,
};

/* ===== ICoreWebView2BasicAuthenticationRequestedEventArgs ===== */

static const IID *const ICoreWebView2BasicAuthenticationRequestedEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2BasicAuthenticationRequestedEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2BasicAuthenticationRequestedEventArgs_get_Uri(ICoreWebView2BasicAuthenticationRequestedEventArgs *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2BasicAuthenticationRequestedEventArgs_get_Challenge(ICoreWebView2BasicAuthenticationRequestedEventArgs *iface, LPWSTR *challenge)
{
    FIXME("iface %p, challenge %p stub!\n", iface, challenge);
    if (challenge) *challenge = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2BasicAuthenticationRequestedEventArgs_get_Response(ICoreWebView2BasicAuthenticationRequestedEventArgs *iface, ICoreWebView2BasicAuthenticationResponse **response)
{
    FIXME("iface %p, response %p stub!\n", iface, response);
    if (response) *response = stub_create(&stub_family_ICoreWebView2BasicAuthenticationResponse);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2BasicAuthenticationRequestedEventArgs_get_Cancel(ICoreWebView2BasicAuthenticationRequestedEventArgs *iface, BOOL *cancel)
{
    FIXME("iface %p, cancel %p stub!\n", iface, cancel);
    if (cancel) memset(cancel, 0, sizeof(*cancel));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2BasicAuthenticationRequestedEventArgs_put_Cancel(ICoreWebView2BasicAuthenticationRequestedEventArgs *iface, BOOL cancel)
{
    FIXME("iface %p, cancel %d stub!\n", iface, (int)cancel);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2BasicAuthenticationRequestedEventArgs_GetDeferral(ICoreWebView2BasicAuthenticationRequestedEventArgs *iface, ICoreWebView2Deferral **deferral)
{
    FIXME("iface %p, deferral %p stub!\n", iface, deferral);
    if (deferral) *deferral = stub_create(&stub_family_ICoreWebView2Deferral);
    return S_OK;
}

static const ICoreWebView2BasicAuthenticationRequestedEventArgsVtbl ICoreWebView2BasicAuthenticationRequestedEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2BasicAuthenticationRequestedEventArgs_get_Uri,
    ICoreWebView2BasicAuthenticationRequestedEventArgs_get_Challenge,
    ICoreWebView2BasicAuthenticationRequestedEventArgs_get_Response,
    ICoreWebView2BasicAuthenticationRequestedEventArgs_get_Cancel,
    ICoreWebView2BasicAuthenticationRequestedEventArgs_put_Cancel,
    ICoreWebView2BasicAuthenticationRequestedEventArgs_GetDeferral,
};

const struct stub_family stub_family_ICoreWebView2BasicAuthenticationRequestedEventArgs =
{
    "ICoreWebView2BasicAuthenticationRequestedEventArgs",
    &ICoreWebView2BasicAuthenticationRequestedEventArgs_stub_vtbl,
    ICoreWebView2BasicAuthenticationRequestedEventArgs_iids,
};

/* ===== ICoreWebView2BasicAuthenticationResponse ===== */

static const IID *const ICoreWebView2BasicAuthenticationResponse_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2BasicAuthenticationResponse,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2BasicAuthenticationResponse_get_UserName(ICoreWebView2BasicAuthenticationResponse *iface, LPWSTR *userName)
{
    FIXME("iface %p, userName %p stub!\n", iface, userName);
    if (userName) *userName = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2BasicAuthenticationResponse_put_UserName(ICoreWebView2BasicAuthenticationResponse *iface, LPCWSTR userName)
{
    FIXME("iface %p, userName %s stub!\n", iface, debugstr_w(userName));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2BasicAuthenticationResponse_get_Password(ICoreWebView2BasicAuthenticationResponse *iface, LPWSTR *password)
{
    FIXME("iface %p, password %p stub!\n", iface, password);
    if (password) *password = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2BasicAuthenticationResponse_put_Password(ICoreWebView2BasicAuthenticationResponse *iface, LPCWSTR password)
{
    FIXME("iface %p, password %s stub!\n", iface, debugstr_w(password));
    return S_OK;
}

static const ICoreWebView2BasicAuthenticationResponseVtbl ICoreWebView2BasicAuthenticationResponse_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2BasicAuthenticationResponse_get_UserName,
    ICoreWebView2BasicAuthenticationResponse_put_UserName,
    ICoreWebView2BasicAuthenticationResponse_get_Password,
    ICoreWebView2BasicAuthenticationResponse_put_Password,
};

const struct stub_family stub_family_ICoreWebView2BasicAuthenticationResponse =
{
    "ICoreWebView2BasicAuthenticationResponse",
    &ICoreWebView2BasicAuthenticationResponse_stub_vtbl,
    ICoreWebView2BasicAuthenticationResponse_iids,
};

/* ===== ICoreWebView2BrowserExtension ===== */

static const IID *const ICoreWebView2BrowserExtension_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2BrowserExtension,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2BrowserExtension_get_Id(ICoreWebView2BrowserExtension *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2BrowserExtension_get_Name(ICoreWebView2BrowserExtension *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2BrowserExtension_Remove(ICoreWebView2BrowserExtension *iface, ICoreWebView2BrowserExtensionRemoveCompletedHandler *handler)
{
    FIXME("iface %p, handler %p stub!\n", iface, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2BrowserExtensionRemoveCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2BrowserExtension_get_IsEnabled(ICoreWebView2BrowserExtension *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2BrowserExtension_Enable(ICoreWebView2BrowserExtension *iface, BOOL isEnabled, ICoreWebView2BrowserExtensionEnableCompletedHandler *handler)
{
    FIXME("iface %p, isEnabled %d, handler %p stub!\n", iface, (int)isEnabled, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2BrowserExtensionEnableCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static const ICoreWebView2BrowserExtensionVtbl ICoreWebView2BrowserExtension_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2BrowserExtension_get_Id,
    ICoreWebView2BrowserExtension_get_Name,
    ICoreWebView2BrowserExtension_Remove,
    ICoreWebView2BrowserExtension_get_IsEnabled,
    ICoreWebView2BrowserExtension_Enable,
};

const struct stub_family stub_family_ICoreWebView2BrowserExtension =
{
    "ICoreWebView2BrowserExtension",
    &ICoreWebView2BrowserExtension_stub_vtbl,
    ICoreWebView2BrowserExtension_iids,
};

/* ===== ICoreWebView2Certificate ===== */

static const IID *const ICoreWebView2Certificate_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2Certificate,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2Certificate_get_Subject(ICoreWebView2Certificate *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Certificate_get_Issuer(ICoreWebView2Certificate *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Certificate_get_ValidFrom(ICoreWebView2Certificate *iface, double *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Certificate_get_ValidTo(ICoreWebView2Certificate *iface, double *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Certificate_get_DerEncodedSerialNumber(ICoreWebView2Certificate *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Certificate_get_DisplayName(ICoreWebView2Certificate *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Certificate_ToPemEncoding(ICoreWebView2Certificate *iface, LPWSTR *pemEncodedData)
{
    FIXME("iface %p, pemEncodedData %p stub!\n", iface, pemEncodedData);
    if (pemEncodedData) *pemEncodedData = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Certificate_get_PemEncodedIssuerCertificateChain(ICoreWebView2Certificate *iface, ICoreWebView2StringCollection **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2StringCollection);
    return S_OK;
}

static const ICoreWebView2CertificateVtbl ICoreWebView2Certificate_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2Certificate_get_Subject,
    ICoreWebView2Certificate_get_Issuer,
    ICoreWebView2Certificate_get_ValidFrom,
    ICoreWebView2Certificate_get_ValidTo,
    ICoreWebView2Certificate_get_DerEncodedSerialNumber,
    ICoreWebView2Certificate_get_DisplayName,
    ICoreWebView2Certificate_ToPemEncoding,
    ICoreWebView2Certificate_get_PemEncodedIssuerCertificateChain,
};

const struct stub_family stub_family_ICoreWebView2Certificate =
{
    "ICoreWebView2Certificate",
    &ICoreWebView2Certificate_stub_vtbl,
    ICoreWebView2Certificate_iids,
};

/* ===== ICoreWebView2ClientCertificate ===== */

static const IID *const ICoreWebView2ClientCertificate_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ClientCertificate,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificate_get_Subject(ICoreWebView2ClientCertificate *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificate_get_Issuer(ICoreWebView2ClientCertificate *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificate_get_ValidFrom(ICoreWebView2ClientCertificate *iface, double *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificate_get_ValidTo(ICoreWebView2ClientCertificate *iface, double *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificate_get_DerEncodedSerialNumber(ICoreWebView2ClientCertificate *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificate_get_DisplayName(ICoreWebView2ClientCertificate *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificate_ToPemEncoding(ICoreWebView2ClientCertificate *iface, LPWSTR *pemEncodedData)
{
    FIXME("iface %p, pemEncodedData %p stub!\n", iface, pemEncodedData);
    if (pemEncodedData) *pemEncodedData = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificate_get_PemEncodedIssuerCertificateChain(ICoreWebView2ClientCertificate *iface, ICoreWebView2StringCollection **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2StringCollection);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificate_get_Kind(ICoreWebView2ClientCertificate *iface, COREWEBVIEW2_CLIENT_CERTIFICATE_KIND *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2ClientCertificateVtbl ICoreWebView2ClientCertificate_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ClientCertificate_get_Subject,
    ICoreWebView2ClientCertificate_get_Issuer,
    ICoreWebView2ClientCertificate_get_ValidFrom,
    ICoreWebView2ClientCertificate_get_ValidTo,
    ICoreWebView2ClientCertificate_get_DerEncodedSerialNumber,
    ICoreWebView2ClientCertificate_get_DisplayName,
    ICoreWebView2ClientCertificate_ToPemEncoding,
    ICoreWebView2ClientCertificate_get_PemEncodedIssuerCertificateChain,
    ICoreWebView2ClientCertificate_get_Kind,
};

const struct stub_family stub_family_ICoreWebView2ClientCertificate =
{
    "ICoreWebView2ClientCertificate",
    &ICoreWebView2ClientCertificate_stub_vtbl,
    ICoreWebView2ClientCertificate_iids,
};

/* ===== ICoreWebView2ClientCertificateRequestedEventArgs ===== */

static const IID *const ICoreWebView2ClientCertificateRequestedEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ClientCertificateRequestedEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificateRequestedEventArgs_get_Host(ICoreWebView2ClientCertificateRequestedEventArgs *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificateRequestedEventArgs_get_Port(ICoreWebView2ClientCertificateRequestedEventArgs *iface, int *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificateRequestedEventArgs_get_IsProxy(ICoreWebView2ClientCertificateRequestedEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificateRequestedEventArgs_get_AllowedCertificateAuthorities(ICoreWebView2ClientCertificateRequestedEventArgs *iface, ICoreWebView2StringCollection **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2StringCollection);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificateRequestedEventArgs_get_MutuallyTrustedCertificates(ICoreWebView2ClientCertificateRequestedEventArgs *iface, ICoreWebView2ClientCertificateCollection **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ClientCertificateCollection);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificateRequestedEventArgs_get_SelectedCertificate(ICoreWebView2ClientCertificateRequestedEventArgs *iface, ICoreWebView2ClientCertificate **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ClientCertificate);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificateRequestedEventArgs_put_SelectedCertificate(ICoreWebView2ClientCertificateRequestedEventArgs *iface, ICoreWebView2ClientCertificate *value)
{
    FIXME("iface %p, value %p stub!\n", iface, (void *)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificateRequestedEventArgs_get_Cancel(ICoreWebView2ClientCertificateRequestedEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificateRequestedEventArgs_put_Cancel(ICoreWebView2ClientCertificateRequestedEventArgs *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificateRequestedEventArgs_get_Handled(ICoreWebView2ClientCertificateRequestedEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificateRequestedEventArgs_put_Handled(ICoreWebView2ClientCertificateRequestedEventArgs *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ClientCertificateRequestedEventArgs_GetDeferral(ICoreWebView2ClientCertificateRequestedEventArgs *iface, ICoreWebView2Deferral **deferral)
{
    FIXME("iface %p, deferral %p stub!\n", iface, deferral);
    if (deferral) *deferral = stub_create(&stub_family_ICoreWebView2Deferral);
    return S_OK;
}

static const ICoreWebView2ClientCertificateRequestedEventArgsVtbl ICoreWebView2ClientCertificateRequestedEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ClientCertificateRequestedEventArgs_get_Host,
    ICoreWebView2ClientCertificateRequestedEventArgs_get_Port,
    ICoreWebView2ClientCertificateRequestedEventArgs_get_IsProxy,
    ICoreWebView2ClientCertificateRequestedEventArgs_get_AllowedCertificateAuthorities,
    ICoreWebView2ClientCertificateRequestedEventArgs_get_MutuallyTrustedCertificates,
    ICoreWebView2ClientCertificateRequestedEventArgs_get_SelectedCertificate,
    ICoreWebView2ClientCertificateRequestedEventArgs_put_SelectedCertificate,
    ICoreWebView2ClientCertificateRequestedEventArgs_get_Cancel,
    ICoreWebView2ClientCertificateRequestedEventArgs_put_Cancel,
    ICoreWebView2ClientCertificateRequestedEventArgs_get_Handled,
    ICoreWebView2ClientCertificateRequestedEventArgs_put_Handled,
    ICoreWebView2ClientCertificateRequestedEventArgs_GetDeferral,
};

const struct stub_family stub_family_ICoreWebView2ClientCertificateRequestedEventArgs =
{
    "ICoreWebView2ClientCertificateRequestedEventArgs",
    &ICoreWebView2ClientCertificateRequestedEventArgs_stub_vtbl,
    ICoreWebView2ClientCertificateRequestedEventArgs_iids,
};

/* ===== ICoreWebView2ContextMenuItem ===== */

static const IID *const ICoreWebView2ContextMenuItem_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ContextMenuItem,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItem_get_Name(ICoreWebView2ContextMenuItem *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItem_get_Label(ICoreWebView2ContextMenuItem *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItem_get_CommandId(ICoreWebView2ContextMenuItem *iface, INT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItem_get_ShortcutKeyDescription(ICoreWebView2ContextMenuItem *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItem_get_Icon(ICoreWebView2ContextMenuItem *iface, IStream **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = NULL;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItem_get_Kind(ICoreWebView2ContextMenuItem *iface, COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItem_put_IsEnabled(ICoreWebView2ContextMenuItem *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItem_get_IsEnabled(ICoreWebView2ContextMenuItem *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItem_put_IsChecked(ICoreWebView2ContextMenuItem *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItem_get_IsChecked(ICoreWebView2ContextMenuItem *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItem_get_Children(ICoreWebView2ContextMenuItem *iface, ICoreWebView2ContextMenuItemCollection **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ContextMenuItemCollection);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItem_add_CustomItemSelected(ICoreWebView2ContextMenuItem *iface, ICoreWebView2CustomItemSelectedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItem_remove_CustomItemSelected(ICoreWebView2ContextMenuItem *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static const ICoreWebView2ContextMenuItemVtbl ICoreWebView2ContextMenuItem_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ContextMenuItem_get_Name,
    ICoreWebView2ContextMenuItem_get_Label,
    ICoreWebView2ContextMenuItem_get_CommandId,
    ICoreWebView2ContextMenuItem_get_ShortcutKeyDescription,
    ICoreWebView2ContextMenuItem_get_Icon,
    ICoreWebView2ContextMenuItem_get_Kind,
    ICoreWebView2ContextMenuItem_put_IsEnabled,
    ICoreWebView2ContextMenuItem_get_IsEnabled,
    ICoreWebView2ContextMenuItem_put_IsChecked,
    ICoreWebView2ContextMenuItem_get_IsChecked,
    ICoreWebView2ContextMenuItem_get_Children,
    ICoreWebView2ContextMenuItem_add_CustomItemSelected,
    ICoreWebView2ContextMenuItem_remove_CustomItemSelected,
};

const struct stub_family stub_family_ICoreWebView2ContextMenuItem =
{
    "ICoreWebView2ContextMenuItem",
    &ICoreWebView2ContextMenuItem_stub_vtbl,
    ICoreWebView2ContextMenuItem_iids,
};

/* ===== ICoreWebView2ContextMenuTarget ===== */

static const IID *const ICoreWebView2ContextMenuTarget_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ContextMenuTarget,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuTarget_get_Kind(ICoreWebView2ContextMenuTarget *iface, COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuTarget_get_IsEditable(ICoreWebView2ContextMenuTarget *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuTarget_get_IsRequestedForMainFrame(ICoreWebView2ContextMenuTarget *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuTarget_get_PageUri(ICoreWebView2ContextMenuTarget *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuTarget_get_FrameUri(ICoreWebView2ContextMenuTarget *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuTarget_get_HasLinkUri(ICoreWebView2ContextMenuTarget *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuTarget_get_LinkUri(ICoreWebView2ContextMenuTarget *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuTarget_get_HasLinkText(ICoreWebView2ContextMenuTarget *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuTarget_get_LinkText(ICoreWebView2ContextMenuTarget *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuTarget_get_HasSourceUri(ICoreWebView2ContextMenuTarget *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuTarget_get_SourceUri(ICoreWebView2ContextMenuTarget *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuTarget_get_HasSelection(ICoreWebView2ContextMenuTarget *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuTarget_get_SelectionText(ICoreWebView2ContextMenuTarget *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static const ICoreWebView2ContextMenuTargetVtbl ICoreWebView2ContextMenuTarget_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ContextMenuTarget_get_Kind,
    ICoreWebView2ContextMenuTarget_get_IsEditable,
    ICoreWebView2ContextMenuTarget_get_IsRequestedForMainFrame,
    ICoreWebView2ContextMenuTarget_get_PageUri,
    ICoreWebView2ContextMenuTarget_get_FrameUri,
    ICoreWebView2ContextMenuTarget_get_HasLinkUri,
    ICoreWebView2ContextMenuTarget_get_LinkUri,
    ICoreWebView2ContextMenuTarget_get_HasLinkText,
    ICoreWebView2ContextMenuTarget_get_LinkText,
    ICoreWebView2ContextMenuTarget_get_HasSourceUri,
    ICoreWebView2ContextMenuTarget_get_SourceUri,
    ICoreWebView2ContextMenuTarget_get_HasSelection,
    ICoreWebView2ContextMenuTarget_get_SelectionText,
};

const struct stub_family stub_family_ICoreWebView2ContextMenuTarget =
{
    "ICoreWebView2ContextMenuTarget",
    &ICoreWebView2ContextMenuTarget_stub_vtbl,
    ICoreWebView2ContextMenuTarget_iids,
};

/* ===== ICoreWebView2ContextMenuRequestedEventArgs ===== */

static const IID *const ICoreWebView2ContextMenuRequestedEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ContextMenuRequestedEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuRequestedEventArgs_get_MenuItems(ICoreWebView2ContextMenuRequestedEventArgs *iface, ICoreWebView2ContextMenuItemCollection **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ContextMenuItemCollection);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuRequestedEventArgs_get_ContextMenuTarget(ICoreWebView2ContextMenuRequestedEventArgs *iface, ICoreWebView2ContextMenuTarget **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ContextMenuTarget);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuRequestedEventArgs_get_Location(ICoreWebView2ContextMenuRequestedEventArgs *iface, POINT *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuRequestedEventArgs_put_SelectedCommandId(ICoreWebView2ContextMenuRequestedEventArgs *iface, INT32 value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuRequestedEventArgs_get_SelectedCommandId(ICoreWebView2ContextMenuRequestedEventArgs *iface, INT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuRequestedEventArgs_put_Handled(ICoreWebView2ContextMenuRequestedEventArgs *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuRequestedEventArgs_get_Handled(ICoreWebView2ContextMenuRequestedEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuRequestedEventArgs_GetDeferral(ICoreWebView2ContextMenuRequestedEventArgs *iface, ICoreWebView2Deferral **deferral)
{
    FIXME("iface %p, deferral %p stub!\n", iface, deferral);
    if (deferral) *deferral = stub_create(&stub_family_ICoreWebView2Deferral);
    return S_OK;
}

static const ICoreWebView2ContextMenuRequestedEventArgsVtbl ICoreWebView2ContextMenuRequestedEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ContextMenuRequestedEventArgs_get_MenuItems,
    ICoreWebView2ContextMenuRequestedEventArgs_get_ContextMenuTarget,
    ICoreWebView2ContextMenuRequestedEventArgs_get_Location,
    ICoreWebView2ContextMenuRequestedEventArgs_put_SelectedCommandId,
    ICoreWebView2ContextMenuRequestedEventArgs_get_SelectedCommandId,
    ICoreWebView2ContextMenuRequestedEventArgs_put_Handled,
    ICoreWebView2ContextMenuRequestedEventArgs_get_Handled,
    ICoreWebView2ContextMenuRequestedEventArgs_GetDeferral,
};

const struct stub_family stub_family_ICoreWebView2ContextMenuRequestedEventArgs =
{
    "ICoreWebView2ContextMenuRequestedEventArgs",
    &ICoreWebView2ContextMenuRequestedEventArgs_stub_vtbl,
    ICoreWebView2ContextMenuRequestedEventArgs_iids,
};

/* ===== ICoreWebView2Cookie ===== */

static const IID *const ICoreWebView2Cookie_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2Cookie,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2Cookie_get_Name(ICoreWebView2Cookie *iface, LPWSTR *name)
{
    FIXME("iface %p, name %p stub!\n", iface, name);
    if (name) *name = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Cookie_get_Value(ICoreWebView2Cookie *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Cookie_put_Value(ICoreWebView2Cookie *iface, LPCWSTR value)
{
    FIXME("iface %p, value %s stub!\n", iface, debugstr_w(value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Cookie_get_Domain(ICoreWebView2Cookie *iface, LPWSTR *domain)
{
    FIXME("iface %p, domain %p stub!\n", iface, domain);
    if (domain) *domain = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Cookie_get_Path(ICoreWebView2Cookie *iface, LPWSTR *path)
{
    FIXME("iface %p, path %p stub!\n", iface, path);
    if (path) *path = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Cookie_get_Expires(ICoreWebView2Cookie *iface, double *expires)
{
    FIXME("iface %p, expires %p stub!\n", iface, expires);
    if (expires) memset(expires, 0, sizeof(*expires));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Cookie_put_Expires(ICoreWebView2Cookie *iface, double expires)
{
    FIXME("iface %p, expires %f stub!\n", iface, expires);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Cookie_get_IsHttpOnly(ICoreWebView2Cookie *iface, BOOL *isHttpOnly)
{
    FIXME("iface %p, isHttpOnly %p stub!\n", iface, isHttpOnly);
    if (isHttpOnly) memset(isHttpOnly, 0, sizeof(*isHttpOnly));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Cookie_put_IsHttpOnly(ICoreWebView2Cookie *iface, BOOL isHttpOnly)
{
    FIXME("iface %p, isHttpOnly %d stub!\n", iface, (int)isHttpOnly);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Cookie_get_SameSite(ICoreWebView2Cookie *iface, COREWEBVIEW2_COOKIE_SAME_SITE_KIND *sameSite)
{
    FIXME("iface %p, sameSite %p stub!\n", iface, sameSite);
    if (sameSite) memset(sameSite, 0, sizeof(*sameSite));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Cookie_put_SameSite(ICoreWebView2Cookie *iface, COREWEBVIEW2_COOKIE_SAME_SITE_KIND sameSite)
{
    FIXME("iface %p, sameSite %d stub!\n", iface, (int)sameSite);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Cookie_get_IsSecure(ICoreWebView2Cookie *iface, BOOL *isSecure)
{
    FIXME("iface %p, isSecure %p stub!\n", iface, isSecure);
    if (isSecure) memset(isSecure, 0, sizeof(*isSecure));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Cookie_put_IsSecure(ICoreWebView2Cookie *iface, BOOL isSecure)
{
    FIXME("iface %p, isSecure %d stub!\n", iface, (int)isSecure);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Cookie_get_IsSession(ICoreWebView2Cookie *iface, BOOL *isSession)
{
    FIXME("iface %p, isSession %p stub!\n", iface, isSession);
    if (isSession) memset(isSession, 0, sizeof(*isSession));
    return S_OK;
}

static const ICoreWebView2CookieVtbl ICoreWebView2Cookie_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2Cookie_get_Name,
    ICoreWebView2Cookie_get_Value,
    ICoreWebView2Cookie_put_Value,
    ICoreWebView2Cookie_get_Domain,
    ICoreWebView2Cookie_get_Path,
    ICoreWebView2Cookie_get_Expires,
    ICoreWebView2Cookie_put_Expires,
    ICoreWebView2Cookie_get_IsHttpOnly,
    ICoreWebView2Cookie_put_IsHttpOnly,
    ICoreWebView2Cookie_get_SameSite,
    ICoreWebView2Cookie_put_SameSite,
    ICoreWebView2Cookie_get_IsSecure,
    ICoreWebView2Cookie_put_IsSecure,
    ICoreWebView2Cookie_get_IsSession,
};

const struct stub_family stub_family_ICoreWebView2Cookie =
{
    "ICoreWebView2Cookie",
    &ICoreWebView2Cookie_stub_vtbl,
    ICoreWebView2Cookie_iids,
};

/* ===== ICoreWebView2CookieManager ===== */

static const IID *const ICoreWebView2CookieManager_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2CookieManager,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2CookieManager_CreateCookie(ICoreWebView2CookieManager *iface, LPCWSTR name, LPCWSTR value, LPCWSTR domain, LPCWSTR path, ICoreWebView2Cookie **cookie)
{
    FIXME("iface %p, name %s, value %s, domain %s, path %s, cookie %p stub!\n", iface, debugstr_w(name), debugstr_w(value), debugstr_w(domain), debugstr_w(path), cookie);
    if (cookie) *cookie = stub_create(&stub_family_ICoreWebView2Cookie);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CookieManager_CopyCookie(ICoreWebView2CookieManager *iface, ICoreWebView2Cookie *cookieParam, ICoreWebView2Cookie **cookie)
{
    FIXME("iface %p, cookieParam %p, cookie %p stub!\n", iface, (void *)cookieParam, cookie);
    if (cookie) *cookie = stub_create(&stub_family_ICoreWebView2Cookie);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CookieManager_GetCookies(ICoreWebView2CookieManager *iface, LPCWSTR uri, ICoreWebView2GetCookiesCompletedHandler *handler)
{
    FIXME("iface %p, uri %s, handler %p stub!\n", iface, debugstr_w(uri), (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2GetCookiesCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CookieManager_AddOrUpdateCookie(ICoreWebView2CookieManager *iface, ICoreWebView2Cookie *cookie)
{
    FIXME("iface %p, cookie %p stub!\n", iface, (void *)cookie);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CookieManager_DeleteCookie(ICoreWebView2CookieManager *iface, ICoreWebView2Cookie *cookie)
{
    FIXME("iface %p, cookie %p stub!\n", iface, (void *)cookie);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CookieManager_DeleteCookies(ICoreWebView2CookieManager *iface, LPCWSTR name, LPCWSTR uri)
{
    FIXME("iface %p, name %s, uri %s stub!\n", iface, debugstr_w(name), debugstr_w(uri));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CookieManager_DeleteCookiesWithDomainAndPath(ICoreWebView2CookieManager *iface, LPCWSTR name, LPCWSTR domain, LPCWSTR path)
{
    FIXME("iface %p, name %s, domain %s, path %s stub!\n", iface, debugstr_w(name), debugstr_w(domain), debugstr_w(path));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CookieManager_DeleteAllCookies(ICoreWebView2CookieManager *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static const ICoreWebView2CookieManagerVtbl ICoreWebView2CookieManager_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2CookieManager_CreateCookie,
    ICoreWebView2CookieManager_CopyCookie,
    ICoreWebView2CookieManager_GetCookies,
    ICoreWebView2CookieManager_AddOrUpdateCookie,
    ICoreWebView2CookieManager_DeleteCookie,
    ICoreWebView2CookieManager_DeleteCookies,
    ICoreWebView2CookieManager_DeleteCookiesWithDomainAndPath,
    ICoreWebView2CookieManager_DeleteAllCookies,
};

const struct stub_family stub_family_ICoreWebView2CookieManager =
{
    "ICoreWebView2CookieManager",
    &ICoreWebView2CookieManager_stub_vtbl,
    ICoreWebView2CookieManager_iids,
};

/* ===== ICoreWebView2DownloadOperation ===== */

static const IID *const ICoreWebView2DownloadOperation_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2DownloadOperation,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_add_BytesReceivedChanged(ICoreWebView2DownloadOperation *iface, ICoreWebView2BytesReceivedChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_remove_BytesReceivedChanged(ICoreWebView2DownloadOperation *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_add_EstimatedEndTimeChanged(ICoreWebView2DownloadOperation *iface, ICoreWebView2EstimatedEndTimeChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_remove_EstimatedEndTimeChanged(ICoreWebView2DownloadOperation *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_add_StateChanged(ICoreWebView2DownloadOperation *iface, ICoreWebView2StateChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_remove_StateChanged(ICoreWebView2DownloadOperation *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_get_Uri(ICoreWebView2DownloadOperation *iface, LPWSTR *uri)
{
    FIXME("iface %p, uri %p stub!\n", iface, uri);
    if (uri) *uri = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_get_ContentDisposition(ICoreWebView2DownloadOperation *iface, LPWSTR *contentDisposition)
{
    FIXME("iface %p, contentDisposition %p stub!\n", iface, contentDisposition);
    if (contentDisposition) *contentDisposition = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_get_MimeType(ICoreWebView2DownloadOperation *iface, LPWSTR *mimeType)
{
    FIXME("iface %p, mimeType %p stub!\n", iface, mimeType);
    if (mimeType) *mimeType = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_get_TotalBytesToReceive(ICoreWebView2DownloadOperation *iface, INT64 *totalBytesToReceive)
{
    FIXME("iface %p, totalBytesToReceive %p stub!\n", iface, totalBytesToReceive);
    if (totalBytesToReceive) memset(totalBytesToReceive, 0, sizeof(*totalBytesToReceive));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_get_BytesReceived(ICoreWebView2DownloadOperation *iface, INT64 *bytesReceived)
{
    FIXME("iface %p, bytesReceived %p stub!\n", iface, bytesReceived);
    if (bytesReceived) memset(bytesReceived, 0, sizeof(*bytesReceived));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_get_EstimatedEndTime(ICoreWebView2DownloadOperation *iface, LPWSTR *estimatedEndTime)
{
    FIXME("iface %p, estimatedEndTime %p stub!\n", iface, estimatedEndTime);
    if (estimatedEndTime) *estimatedEndTime = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_get_ResultFilePath(ICoreWebView2DownloadOperation *iface, LPWSTR *resultFilePath)
{
    FIXME("iface %p, resultFilePath %p stub!\n", iface, resultFilePath);
    if (resultFilePath) *resultFilePath = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_get_State(ICoreWebView2DownloadOperation *iface, COREWEBVIEW2_DOWNLOAD_STATE *downloadState)
{
    FIXME("iface %p, downloadState %p stub!\n", iface, downloadState);
    if (downloadState) memset(downloadState, 0, sizeof(*downloadState));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_get_InterruptReason(ICoreWebView2DownloadOperation *iface, COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON *interruptReason)
{
    FIXME("iface %p, interruptReason %p stub!\n", iface, interruptReason);
    if (interruptReason) memset(interruptReason, 0, sizeof(*interruptReason));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_Cancel(ICoreWebView2DownloadOperation *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_Pause(ICoreWebView2DownloadOperation *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_Resume(ICoreWebView2DownloadOperation *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadOperation_get_CanResume(ICoreWebView2DownloadOperation *iface, BOOL *canResume)
{
    FIXME("iface %p, canResume %p stub!\n", iface, canResume);
    if (canResume) memset(canResume, 0, sizeof(*canResume));
    return S_OK;
}

static const ICoreWebView2DownloadOperationVtbl ICoreWebView2DownloadOperation_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2DownloadOperation_add_BytesReceivedChanged,
    ICoreWebView2DownloadOperation_remove_BytesReceivedChanged,
    ICoreWebView2DownloadOperation_add_EstimatedEndTimeChanged,
    ICoreWebView2DownloadOperation_remove_EstimatedEndTimeChanged,
    ICoreWebView2DownloadOperation_add_StateChanged,
    ICoreWebView2DownloadOperation_remove_StateChanged,
    ICoreWebView2DownloadOperation_get_Uri,
    ICoreWebView2DownloadOperation_get_ContentDisposition,
    ICoreWebView2DownloadOperation_get_MimeType,
    ICoreWebView2DownloadOperation_get_TotalBytesToReceive,
    ICoreWebView2DownloadOperation_get_BytesReceived,
    ICoreWebView2DownloadOperation_get_EstimatedEndTime,
    ICoreWebView2DownloadOperation_get_ResultFilePath,
    ICoreWebView2DownloadOperation_get_State,
    ICoreWebView2DownloadOperation_get_InterruptReason,
    ICoreWebView2DownloadOperation_Cancel,
    ICoreWebView2DownloadOperation_Pause,
    ICoreWebView2DownloadOperation_Resume,
    ICoreWebView2DownloadOperation_get_CanResume,
};

const struct stub_family stub_family_ICoreWebView2DownloadOperation =
{
    "ICoreWebView2DownloadOperation",
    &ICoreWebView2DownloadOperation_stub_vtbl,
    ICoreWebView2DownloadOperation_iids,
};

/* ===== ICoreWebView2DownloadStartingEventArgs ===== */

static const IID *const ICoreWebView2DownloadStartingEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2DownloadStartingEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadStartingEventArgs_get_DownloadOperation(ICoreWebView2DownloadStartingEventArgs *iface, ICoreWebView2DownloadOperation **downloadOperation)
{
    FIXME("iface %p, downloadOperation %p stub!\n", iface, downloadOperation);
    if (downloadOperation) *downloadOperation = stub_create(&stub_family_ICoreWebView2DownloadOperation);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadStartingEventArgs_get_Cancel(ICoreWebView2DownloadStartingEventArgs *iface, BOOL *cancel)
{
    FIXME("iface %p, cancel %p stub!\n", iface, cancel);
    if (cancel) memset(cancel, 0, sizeof(*cancel));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadStartingEventArgs_put_Cancel(ICoreWebView2DownloadStartingEventArgs *iface, BOOL cancel)
{
    FIXME("iface %p, cancel %d stub!\n", iface, (int)cancel);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadStartingEventArgs_get_ResultFilePath(ICoreWebView2DownloadStartingEventArgs *iface, LPWSTR *resultFilePath)
{
    FIXME("iface %p, resultFilePath %p stub!\n", iface, resultFilePath);
    if (resultFilePath) *resultFilePath = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadStartingEventArgs_put_ResultFilePath(ICoreWebView2DownloadStartingEventArgs *iface, LPCWSTR resultFilePath)
{
    FIXME("iface %p, resultFilePath %s stub!\n", iface, debugstr_w(resultFilePath));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadStartingEventArgs_get_Handled(ICoreWebView2DownloadStartingEventArgs *iface, BOOL *handled)
{
    FIXME("iface %p, handled %p stub!\n", iface, handled);
    if (handled) memset(handled, 0, sizeof(*handled));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadStartingEventArgs_put_Handled(ICoreWebView2DownloadStartingEventArgs *iface, BOOL handled)
{
    FIXME("iface %p, handled %d stub!\n", iface, (int)handled);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DownloadStartingEventArgs_GetDeferral(ICoreWebView2DownloadStartingEventArgs *iface, ICoreWebView2Deferral **deferral)
{
    FIXME("iface %p, deferral %p stub!\n", iface, deferral);
    if (deferral) *deferral = stub_create(&stub_family_ICoreWebView2Deferral);
    return S_OK;
}

static const ICoreWebView2DownloadStartingEventArgsVtbl ICoreWebView2DownloadStartingEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2DownloadStartingEventArgs_get_DownloadOperation,
    ICoreWebView2DownloadStartingEventArgs_get_Cancel,
    ICoreWebView2DownloadStartingEventArgs_put_Cancel,
    ICoreWebView2DownloadStartingEventArgs_get_ResultFilePath,
    ICoreWebView2DownloadStartingEventArgs_put_ResultFilePath,
    ICoreWebView2DownloadStartingEventArgs_get_Handled,
    ICoreWebView2DownloadStartingEventArgs_put_Handled,
    ICoreWebView2DownloadStartingEventArgs_GetDeferral,
};

const struct stub_family stub_family_ICoreWebView2DownloadStartingEventArgs =
{
    "ICoreWebView2DownloadStartingEventArgs",
    &ICoreWebView2DownloadStartingEventArgs_stub_vtbl,
    ICoreWebView2DownloadStartingEventArgs_iids,
};

/* ===== ICoreWebView2ExecuteScriptResult ===== */

static const IID *const ICoreWebView2ExecuteScriptResult_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ExecuteScriptResult,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ExecuteScriptResult_get_Succeeded(ICoreWebView2ExecuteScriptResult *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ExecuteScriptResult_get_ResultAsJson(ICoreWebView2ExecuteScriptResult *iface, LPWSTR *jsonResult)
{
    FIXME("iface %p, jsonResult %p stub!\n", iface, jsonResult);
    if (jsonResult) *jsonResult = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ExecuteScriptResult_TryGetResultAsString(ICoreWebView2ExecuteScriptResult *iface, LPWSTR *stringResult, BOOL *value)
{
    FIXME("iface %p, stringResult %p, value %p stub!\n", iface, stringResult, value);
    if (stringResult) *stringResult = stub_strdup(L"");
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ExecuteScriptResult_get_Exception(ICoreWebView2ExecuteScriptResult *iface, ICoreWebView2ScriptException **exception)
{
    FIXME("iface %p, exception %p stub!\n", iface, exception);
    if (exception) *exception = stub_create(&stub_family_ICoreWebView2ScriptException);
    return S_OK;
}

static const ICoreWebView2ExecuteScriptResultVtbl ICoreWebView2ExecuteScriptResult_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ExecuteScriptResult_get_Succeeded,
    ICoreWebView2ExecuteScriptResult_get_ResultAsJson,
    ICoreWebView2ExecuteScriptResult_TryGetResultAsString,
    ICoreWebView2ExecuteScriptResult_get_Exception,
};

const struct stub_family stub_family_ICoreWebView2ExecuteScriptResult =
{
    "ICoreWebView2ExecuteScriptResult",
    &ICoreWebView2ExecuteScriptResult_stub_vtbl,
    ICoreWebView2ExecuteScriptResult_iids,
};

/* ===== ICoreWebView2FrameInfo2 ===== */

static const IID *const ICoreWebView2FrameInfo2_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2FrameInfo,
    &IID_ICoreWebView2FrameInfo2,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2FrameInfo_get_Name(ICoreWebView2FrameInfo2 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FrameInfo_get_Source(ICoreWebView2FrameInfo2 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FrameInfo2_get_ParentFrameInfo(ICoreWebView2FrameInfo2 *iface, ICoreWebView2FrameInfo **frameInfo)
{
    FIXME("iface %p, frameInfo %p stub!\n", iface, frameInfo);
    if (frameInfo) *frameInfo = stub_create(&stub_family_ICoreWebView2FrameInfo2);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FrameInfo2_get_FrameId(ICoreWebView2FrameInfo2 *iface, UINT32 *id)
{
    FIXME("iface %p, id %p stub!\n", iface, id);
    if (id) memset(id, 0, sizeof(*id));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FrameInfo2_get_FrameKind(ICoreWebView2FrameInfo2 *iface, COREWEBVIEW2_FRAME_KIND *kind)
{
    FIXME("iface %p, kind %p stub!\n", iface, kind);
    if (kind) memset(kind, 0, sizeof(*kind));
    return S_OK;
}

static const ICoreWebView2FrameInfo2Vtbl ICoreWebView2FrameInfo2_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2FrameInfo_get_Name,
    ICoreWebView2FrameInfo_get_Source,
    ICoreWebView2FrameInfo2_get_ParentFrameInfo,
    ICoreWebView2FrameInfo2_get_FrameId,
    ICoreWebView2FrameInfo2_get_FrameKind,
};

const struct stub_family stub_family_ICoreWebView2FrameInfo2 =
{
    "ICoreWebView2FrameInfo2",
    &ICoreWebView2FrameInfo2_stub_vtbl,
    ICoreWebView2FrameInfo2_iids,
};

/* ===== ICoreWebView2HttpHeadersCollectionIterator ===== */

static const IID *const ICoreWebView2HttpHeadersCollectionIterator_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2HttpHeadersCollectionIterator,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2HttpHeadersCollectionIterator_GetCurrentHeader(ICoreWebView2HttpHeadersCollectionIterator *iface, LPWSTR *name, LPWSTR *value)
{
    FIXME("iface %p, name %p, value %p stub!\n", iface, name, value);
    if (name) *name = stub_strdup(L"");
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2HttpHeadersCollectionIterator_get_HasCurrentHeader(ICoreWebView2HttpHeadersCollectionIterator *iface, BOOL *hasCurrent)
{
    FIXME("iface %p, hasCurrent %p stub!\n", iface, hasCurrent);
    if (hasCurrent) memset(hasCurrent, 0, sizeof(*hasCurrent));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2HttpHeadersCollectionIterator_MoveNext(ICoreWebView2HttpHeadersCollectionIterator *iface, BOOL *hasNext)
{
    FIXME("iface %p, hasNext %p stub!\n", iface, hasNext);
    if (hasNext) memset(hasNext, 0, sizeof(*hasNext));
    return S_OK;
}

static const ICoreWebView2HttpHeadersCollectionIteratorVtbl ICoreWebView2HttpHeadersCollectionIterator_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2HttpHeadersCollectionIterator_GetCurrentHeader,
    ICoreWebView2HttpHeadersCollectionIterator_get_HasCurrentHeader,
    ICoreWebView2HttpHeadersCollectionIterator_MoveNext,
};

const struct stub_family stub_family_ICoreWebView2HttpHeadersCollectionIterator =
{
    "ICoreWebView2HttpHeadersCollectionIterator",
    &ICoreWebView2HttpHeadersCollectionIterator_stub_vtbl,
    ICoreWebView2HttpHeadersCollectionIterator_iids,
};

/* ===== ICoreWebView2LaunchingExternalUriSchemeEventArgs ===== */

static const IID *const ICoreWebView2LaunchingExternalUriSchemeEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2LaunchingExternalUriSchemeEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2LaunchingExternalUriSchemeEventArgs_get_Uri(ICoreWebView2LaunchingExternalUriSchemeEventArgs *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2LaunchingExternalUriSchemeEventArgs_get_InitiatingOrigin(ICoreWebView2LaunchingExternalUriSchemeEventArgs *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2LaunchingExternalUriSchemeEventArgs_get_IsUserInitiated(ICoreWebView2LaunchingExternalUriSchemeEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2LaunchingExternalUriSchemeEventArgs_get_Cancel(ICoreWebView2LaunchingExternalUriSchemeEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2LaunchingExternalUriSchemeEventArgs_put_Cancel(ICoreWebView2LaunchingExternalUriSchemeEventArgs *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2LaunchingExternalUriSchemeEventArgs_GetDeferral(ICoreWebView2LaunchingExternalUriSchemeEventArgs *iface, ICoreWebView2Deferral **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2Deferral);
    return S_OK;
}

static const ICoreWebView2LaunchingExternalUriSchemeEventArgsVtbl ICoreWebView2LaunchingExternalUriSchemeEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2LaunchingExternalUriSchemeEventArgs_get_Uri,
    ICoreWebView2LaunchingExternalUriSchemeEventArgs_get_InitiatingOrigin,
    ICoreWebView2LaunchingExternalUriSchemeEventArgs_get_IsUserInitiated,
    ICoreWebView2LaunchingExternalUriSchemeEventArgs_get_Cancel,
    ICoreWebView2LaunchingExternalUriSchemeEventArgs_put_Cancel,
    ICoreWebView2LaunchingExternalUriSchemeEventArgs_GetDeferral,
};

const struct stub_family stub_family_ICoreWebView2LaunchingExternalUriSchemeEventArgs =
{
    "ICoreWebView2LaunchingExternalUriSchemeEventArgs",
    &ICoreWebView2LaunchingExternalUriSchemeEventArgs_stub_vtbl,
    ICoreWebView2LaunchingExternalUriSchemeEventArgs_iids,
};

/* ===== ICoreWebView2MoveFocusRequestedEventArgs ===== */

static const IID *const ICoreWebView2MoveFocusRequestedEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2MoveFocusRequestedEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2MoveFocusRequestedEventArgs_get_Reason(ICoreWebView2MoveFocusRequestedEventArgs *iface, COREWEBVIEW2_MOVE_FOCUS_REASON *reason)
{
    FIXME("iface %p, reason %p stub!\n", iface, reason);
    if (reason) memset(reason, 0, sizeof(*reason));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2MoveFocusRequestedEventArgs_get_Handled(ICoreWebView2MoveFocusRequestedEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2MoveFocusRequestedEventArgs_put_Handled(ICoreWebView2MoveFocusRequestedEventArgs *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static const ICoreWebView2MoveFocusRequestedEventArgsVtbl ICoreWebView2MoveFocusRequestedEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2MoveFocusRequestedEventArgs_get_Reason,
    ICoreWebView2MoveFocusRequestedEventArgs_get_Handled,
    ICoreWebView2MoveFocusRequestedEventArgs_put_Handled,
};

const struct stub_family stub_family_ICoreWebView2MoveFocusRequestedEventArgs =
{
    "ICoreWebView2MoveFocusRequestedEventArgs",
    &ICoreWebView2MoveFocusRequestedEventArgs_stub_vtbl,
    ICoreWebView2MoveFocusRequestedEventArgs_iids,
};

/* ===== ICoreWebView2PermissionSettingCollectionView ===== */

static const IID *const ICoreWebView2PermissionSettingCollectionView_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2PermissionSettingCollectionView,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2PermissionSettingCollectionView_GetValueAtIndex(ICoreWebView2PermissionSettingCollectionView *iface, UINT32 index, ICoreWebView2PermissionSetting **permissionSetting)
{
    FIXME("iface %p, index %d, permissionSetting %p stub!\n", iface, (int)index, permissionSetting);
    if (permissionSetting) *permissionSetting = stub_create(&stub_family_ICoreWebView2PermissionSetting);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PermissionSettingCollectionView_get_Count(ICoreWebView2PermissionSettingCollectionView *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2PermissionSettingCollectionViewVtbl ICoreWebView2PermissionSettingCollectionView_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2PermissionSettingCollectionView_GetValueAtIndex,
    ICoreWebView2PermissionSettingCollectionView_get_Count,
};

const struct stub_family stub_family_ICoreWebView2PermissionSettingCollectionView =
{
    "ICoreWebView2PermissionSettingCollectionView",
    &ICoreWebView2PermissionSettingCollectionView_stub_vtbl,
    ICoreWebView2PermissionSettingCollectionView_iids,
};

/* ===== ICoreWebView2PointerInfo ===== */

static const IID *const ICoreWebView2PointerInfo_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2PointerInfo,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_PointerKind(ICoreWebView2PointerInfo *iface, DWORD *pointerKind)
{
    FIXME("iface %p, pointerKind %p stub!\n", iface, pointerKind);
    if (pointerKind) memset(pointerKind, 0, sizeof(*pointerKind));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_PointerKind(ICoreWebView2PointerInfo *iface, DWORD pointerKind)
{
    FIXME("iface %p, pointerKind %d stub!\n", iface, (int)pointerKind);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_PointerId(ICoreWebView2PointerInfo *iface, UINT32 *pointerId)
{
    FIXME("iface %p, pointerId %p stub!\n", iface, pointerId);
    if (pointerId) memset(pointerId, 0, sizeof(*pointerId));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_PointerId(ICoreWebView2PointerInfo *iface, UINT32 pointerId)
{
    FIXME("iface %p, pointerId %d stub!\n", iface, (int)pointerId);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_FrameId(ICoreWebView2PointerInfo *iface, UINT32 *frameId)
{
    FIXME("iface %p, frameId %p stub!\n", iface, frameId);
    if (frameId) memset(frameId, 0, sizeof(*frameId));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_FrameId(ICoreWebView2PointerInfo *iface, UINT32 frameId)
{
    FIXME("iface %p, frameId %d stub!\n", iface, (int)frameId);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_PointerFlags(ICoreWebView2PointerInfo *iface, UINT32 *pointerFlags)
{
    FIXME("iface %p, pointerFlags %p stub!\n", iface, pointerFlags);
    if (pointerFlags) memset(pointerFlags, 0, sizeof(*pointerFlags));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_PointerFlags(ICoreWebView2PointerInfo *iface, UINT32 pointerFlags)
{
    FIXME("iface %p, pointerFlags %d stub!\n", iface, (int)pointerFlags);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_PointerDeviceRect(ICoreWebView2PointerInfo *iface, RECT *pointerDeviceRect)
{
    FIXME("iface %p, pointerDeviceRect %p stub!\n", iface, pointerDeviceRect);
    if (pointerDeviceRect) memset(pointerDeviceRect, 0, sizeof(*pointerDeviceRect));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_PointerDeviceRect(ICoreWebView2PointerInfo *iface, RECT pointerDeviceRect)
{
    FIXME("iface %p, pointerDeviceRect %s stub!\n", iface, wine_dbgstr_rect(&pointerDeviceRect));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_DisplayRect(ICoreWebView2PointerInfo *iface, RECT *displayRect)
{
    FIXME("iface %p, displayRect %p stub!\n", iface, displayRect);
    if (displayRect) memset(displayRect, 0, sizeof(*displayRect));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_DisplayRect(ICoreWebView2PointerInfo *iface, RECT displayRect)
{
    FIXME("iface %p, displayRect %s stub!\n", iface, wine_dbgstr_rect(&displayRect));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_PixelLocation(ICoreWebView2PointerInfo *iface, POINT *pixelLocation)
{
    FIXME("iface %p, pixelLocation %p stub!\n", iface, pixelLocation);
    if (pixelLocation) memset(pixelLocation, 0, sizeof(*pixelLocation));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_PixelLocation(ICoreWebView2PointerInfo *iface, POINT pixelLocation)
{
    FIXME("iface %p, pixelLocation %s stub!\n", iface, wine_dbgstr_point(&pixelLocation));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_HimetricLocation(ICoreWebView2PointerInfo *iface, POINT *himetricLocation)
{
    FIXME("iface %p, himetricLocation %p stub!\n", iface, himetricLocation);
    if (himetricLocation) memset(himetricLocation, 0, sizeof(*himetricLocation));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_HimetricLocation(ICoreWebView2PointerInfo *iface, POINT himetricLocation)
{
    FIXME("iface %p, himetricLocation %s stub!\n", iface, wine_dbgstr_point(&himetricLocation));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_PixelLocationRaw(ICoreWebView2PointerInfo *iface, POINT *pixelLocationRaw)
{
    FIXME("iface %p, pixelLocationRaw %p stub!\n", iface, pixelLocationRaw);
    if (pixelLocationRaw) memset(pixelLocationRaw, 0, sizeof(*pixelLocationRaw));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_PixelLocationRaw(ICoreWebView2PointerInfo *iface, POINT pixelLocationRaw)
{
    FIXME("iface %p, pixelLocationRaw %s stub!\n", iface, wine_dbgstr_point(&pixelLocationRaw));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_HimetricLocationRaw(ICoreWebView2PointerInfo *iface, POINT *himetricLocationRaw)
{
    FIXME("iface %p, himetricLocationRaw %p stub!\n", iface, himetricLocationRaw);
    if (himetricLocationRaw) memset(himetricLocationRaw, 0, sizeof(*himetricLocationRaw));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_HimetricLocationRaw(ICoreWebView2PointerInfo *iface, POINT himetricLocationRaw)
{
    FIXME("iface %p, himetricLocationRaw %s stub!\n", iface, wine_dbgstr_point(&himetricLocationRaw));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_Time(ICoreWebView2PointerInfo *iface, DWORD *time)
{
    FIXME("iface %p, time %p stub!\n", iface, time);
    if (time) memset(time, 0, sizeof(*time));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_Time(ICoreWebView2PointerInfo *iface, DWORD time)
{
    FIXME("iface %p, time %d stub!\n", iface, (int)time);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_HistoryCount(ICoreWebView2PointerInfo *iface, UINT32 *historyCount)
{
    FIXME("iface %p, historyCount %p stub!\n", iface, historyCount);
    if (historyCount) memset(historyCount, 0, sizeof(*historyCount));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_HistoryCount(ICoreWebView2PointerInfo *iface, UINT32 historyCount)
{
    FIXME("iface %p, historyCount %d stub!\n", iface, (int)historyCount);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_InputData(ICoreWebView2PointerInfo *iface, INT32 *inputData)
{
    FIXME("iface %p, inputData %p stub!\n", iface, inputData);
    if (inputData) memset(inputData, 0, sizeof(*inputData));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_InputData(ICoreWebView2PointerInfo *iface, INT32 inputData)
{
    FIXME("iface %p, inputData %d stub!\n", iface, (int)inputData);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_KeyStates(ICoreWebView2PointerInfo *iface, DWORD *keyStates)
{
    FIXME("iface %p, keyStates %p stub!\n", iface, keyStates);
    if (keyStates) memset(keyStates, 0, sizeof(*keyStates));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_KeyStates(ICoreWebView2PointerInfo *iface, DWORD keyStates)
{
    FIXME("iface %p, keyStates %d stub!\n", iface, (int)keyStates);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_PerformanceCount(ICoreWebView2PointerInfo *iface, UINT64 *performanceCount)
{
    FIXME("iface %p, performanceCount %p stub!\n", iface, performanceCount);
    if (performanceCount) memset(performanceCount, 0, sizeof(*performanceCount));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_PerformanceCount(ICoreWebView2PointerInfo *iface, UINT64 performanceCount)
{
    FIXME("iface %p, performanceCount %s stub!\n", iface, wine_dbgstr_longlong(performanceCount));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_ButtonChangeKind(ICoreWebView2PointerInfo *iface, INT32 *buttonChangeKind)
{
    FIXME("iface %p, buttonChangeKind %p stub!\n", iface, buttonChangeKind);
    if (buttonChangeKind) memset(buttonChangeKind, 0, sizeof(*buttonChangeKind));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_ButtonChangeKind(ICoreWebView2PointerInfo *iface, INT32 buttonChangeKind)
{
    FIXME("iface %p, buttonChangeKind %d stub!\n", iface, (int)buttonChangeKind);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_PenFlags(ICoreWebView2PointerInfo *iface, UINT32 *penFLags)
{
    FIXME("iface %p, penFLags %p stub!\n", iface, penFLags);
    if (penFLags) memset(penFLags, 0, sizeof(*penFLags));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_PenFlags(ICoreWebView2PointerInfo *iface, UINT32 penFLags)
{
    FIXME("iface %p, penFLags %d stub!\n", iface, (int)penFLags);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_PenMask(ICoreWebView2PointerInfo *iface, UINT32 *penMask)
{
    FIXME("iface %p, penMask %p stub!\n", iface, penMask);
    if (penMask) memset(penMask, 0, sizeof(*penMask));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_PenMask(ICoreWebView2PointerInfo *iface, UINT32 penMask)
{
    FIXME("iface %p, penMask %d stub!\n", iface, (int)penMask);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_PenPressure(ICoreWebView2PointerInfo *iface, UINT32 *penPressure)
{
    FIXME("iface %p, penPressure %p stub!\n", iface, penPressure);
    if (penPressure) memset(penPressure, 0, sizeof(*penPressure));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_PenPressure(ICoreWebView2PointerInfo *iface, UINT32 penPressure)
{
    FIXME("iface %p, penPressure %d stub!\n", iface, (int)penPressure);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_PenRotation(ICoreWebView2PointerInfo *iface, UINT32 *penRotation)
{
    FIXME("iface %p, penRotation %p stub!\n", iface, penRotation);
    if (penRotation) memset(penRotation, 0, sizeof(*penRotation));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_PenRotation(ICoreWebView2PointerInfo *iface, UINT32 penRotation)
{
    FIXME("iface %p, penRotation %d stub!\n", iface, (int)penRotation);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_PenTiltX(ICoreWebView2PointerInfo *iface, INT32 *penTiltX)
{
    FIXME("iface %p, penTiltX %p stub!\n", iface, penTiltX);
    if (penTiltX) memset(penTiltX, 0, sizeof(*penTiltX));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_PenTiltX(ICoreWebView2PointerInfo *iface, INT32 penTiltX)
{
    FIXME("iface %p, penTiltX %d stub!\n", iface, (int)penTiltX);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_PenTiltY(ICoreWebView2PointerInfo *iface, INT32 *penTiltY)
{
    FIXME("iface %p, penTiltY %p stub!\n", iface, penTiltY);
    if (penTiltY) memset(penTiltY, 0, sizeof(*penTiltY));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_PenTiltY(ICoreWebView2PointerInfo *iface, INT32 penTiltY)
{
    FIXME("iface %p, penTiltY %d stub!\n", iface, (int)penTiltY);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_TouchFlags(ICoreWebView2PointerInfo *iface, UINT32 *touchFlags)
{
    FIXME("iface %p, touchFlags %p stub!\n", iface, touchFlags);
    if (touchFlags) memset(touchFlags, 0, sizeof(*touchFlags));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_TouchFlags(ICoreWebView2PointerInfo *iface, UINT32 touchFlags)
{
    FIXME("iface %p, touchFlags %d stub!\n", iface, (int)touchFlags);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_TouchMask(ICoreWebView2PointerInfo *iface, UINT32 *touchMask)
{
    FIXME("iface %p, touchMask %p stub!\n", iface, touchMask);
    if (touchMask) memset(touchMask, 0, sizeof(*touchMask));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_TouchMask(ICoreWebView2PointerInfo *iface, UINT32 touchMask)
{
    FIXME("iface %p, touchMask %d stub!\n", iface, (int)touchMask);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_TouchContact(ICoreWebView2PointerInfo *iface, RECT *touchContact)
{
    FIXME("iface %p, touchContact %p stub!\n", iface, touchContact);
    if (touchContact) memset(touchContact, 0, sizeof(*touchContact));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_TouchContact(ICoreWebView2PointerInfo *iface, RECT touchContact)
{
    FIXME("iface %p, touchContact %s stub!\n", iface, wine_dbgstr_rect(&touchContact));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_TouchContactRaw(ICoreWebView2PointerInfo *iface, RECT *touchContactRaw)
{
    FIXME("iface %p, touchContactRaw %p stub!\n", iface, touchContactRaw);
    if (touchContactRaw) memset(touchContactRaw, 0, sizeof(*touchContactRaw));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_TouchContactRaw(ICoreWebView2PointerInfo *iface, RECT touchContactRaw)
{
    FIXME("iface %p, touchContactRaw %s stub!\n", iface, wine_dbgstr_rect(&touchContactRaw));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_TouchOrientation(ICoreWebView2PointerInfo *iface, UINT32 *touchOrientation)
{
    FIXME("iface %p, touchOrientation %p stub!\n", iface, touchOrientation);
    if (touchOrientation) memset(touchOrientation, 0, sizeof(*touchOrientation));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_TouchOrientation(ICoreWebView2PointerInfo *iface, UINT32 touchOrientation)
{
    FIXME("iface %p, touchOrientation %d stub!\n", iface, (int)touchOrientation);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_get_TouchPressure(ICoreWebView2PointerInfo *iface, UINT32 *touchPressure)
{
    FIXME("iface %p, touchPressure %p stub!\n", iface, touchPressure);
    if (touchPressure) memset(touchPressure, 0, sizeof(*touchPressure));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PointerInfo_put_TouchPressure(ICoreWebView2PointerInfo *iface, UINT32 touchPressure)
{
    FIXME("iface %p, touchPressure %d stub!\n", iface, (int)touchPressure);
    return S_OK;
}

static const ICoreWebView2PointerInfoVtbl ICoreWebView2PointerInfo_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2PointerInfo_get_PointerKind,
    ICoreWebView2PointerInfo_put_PointerKind,
    ICoreWebView2PointerInfo_get_PointerId,
    ICoreWebView2PointerInfo_put_PointerId,
    ICoreWebView2PointerInfo_get_FrameId,
    ICoreWebView2PointerInfo_put_FrameId,
    ICoreWebView2PointerInfo_get_PointerFlags,
    ICoreWebView2PointerInfo_put_PointerFlags,
    ICoreWebView2PointerInfo_get_PointerDeviceRect,
    ICoreWebView2PointerInfo_put_PointerDeviceRect,
    ICoreWebView2PointerInfo_get_DisplayRect,
    ICoreWebView2PointerInfo_put_DisplayRect,
    ICoreWebView2PointerInfo_get_PixelLocation,
    ICoreWebView2PointerInfo_put_PixelLocation,
    ICoreWebView2PointerInfo_get_HimetricLocation,
    ICoreWebView2PointerInfo_put_HimetricLocation,
    ICoreWebView2PointerInfo_get_PixelLocationRaw,
    ICoreWebView2PointerInfo_put_PixelLocationRaw,
    ICoreWebView2PointerInfo_get_HimetricLocationRaw,
    ICoreWebView2PointerInfo_put_HimetricLocationRaw,
    ICoreWebView2PointerInfo_get_Time,
    ICoreWebView2PointerInfo_put_Time,
    ICoreWebView2PointerInfo_get_HistoryCount,
    ICoreWebView2PointerInfo_put_HistoryCount,
    ICoreWebView2PointerInfo_get_InputData,
    ICoreWebView2PointerInfo_put_InputData,
    ICoreWebView2PointerInfo_get_KeyStates,
    ICoreWebView2PointerInfo_put_KeyStates,
    ICoreWebView2PointerInfo_get_PerformanceCount,
    ICoreWebView2PointerInfo_put_PerformanceCount,
    ICoreWebView2PointerInfo_get_ButtonChangeKind,
    ICoreWebView2PointerInfo_put_ButtonChangeKind,
    ICoreWebView2PointerInfo_get_PenFlags,
    ICoreWebView2PointerInfo_put_PenFlags,
    ICoreWebView2PointerInfo_get_PenMask,
    ICoreWebView2PointerInfo_put_PenMask,
    ICoreWebView2PointerInfo_get_PenPressure,
    ICoreWebView2PointerInfo_put_PenPressure,
    ICoreWebView2PointerInfo_get_PenRotation,
    ICoreWebView2PointerInfo_put_PenRotation,
    ICoreWebView2PointerInfo_get_PenTiltX,
    ICoreWebView2PointerInfo_put_PenTiltX,
    ICoreWebView2PointerInfo_get_PenTiltY,
    ICoreWebView2PointerInfo_put_PenTiltY,
    ICoreWebView2PointerInfo_get_TouchFlags,
    ICoreWebView2PointerInfo_put_TouchFlags,
    ICoreWebView2PointerInfo_get_TouchMask,
    ICoreWebView2PointerInfo_put_TouchMask,
    ICoreWebView2PointerInfo_get_TouchContact,
    ICoreWebView2PointerInfo_put_TouchContact,
    ICoreWebView2PointerInfo_get_TouchContactRaw,
    ICoreWebView2PointerInfo_put_TouchContactRaw,
    ICoreWebView2PointerInfo_get_TouchOrientation,
    ICoreWebView2PointerInfo_put_TouchOrientation,
    ICoreWebView2PointerInfo_get_TouchPressure,
    ICoreWebView2PointerInfo_put_TouchPressure,
};

const struct stub_family stub_family_ICoreWebView2PointerInfo =
{
    "ICoreWebView2PointerInfo",
    &ICoreWebView2PointerInfo_stub_vtbl,
    ICoreWebView2PointerInfo_iids,
};

/* ===== ICoreWebView2PrintSettings2 ===== */

static const IID *const ICoreWebView2PrintSettings2_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2PrintSettings,
    &IID_ICoreWebView2PrintSettings2,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_get_Orientation(ICoreWebView2PrintSettings2 *iface, COREWEBVIEW2_PRINT_ORIENTATION *orientation)
{
    FIXME("iface %p, orientation %p stub!\n", iface, orientation);
    if (orientation) memset(orientation, 0, sizeof(*orientation));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_put_Orientation(ICoreWebView2PrintSettings2 *iface, COREWEBVIEW2_PRINT_ORIENTATION orientation)
{
    FIXME("iface %p, orientation %d stub!\n", iface, (int)orientation);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_get_ScaleFactor(ICoreWebView2PrintSettings2 *iface, double *scaleFactor)
{
    FIXME("iface %p, scaleFactor %p stub!\n", iface, scaleFactor);
    if (scaleFactor) memset(scaleFactor, 0, sizeof(*scaleFactor));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_put_ScaleFactor(ICoreWebView2PrintSettings2 *iface, double scaleFactor)
{
    FIXME("iface %p, scaleFactor %f stub!\n", iface, scaleFactor);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_get_PageWidth(ICoreWebView2PrintSettings2 *iface, double *pageWidth)
{
    FIXME("iface %p, pageWidth %p stub!\n", iface, pageWidth);
    if (pageWidth) memset(pageWidth, 0, sizeof(*pageWidth));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_put_PageWidth(ICoreWebView2PrintSettings2 *iface, double pageWidth)
{
    FIXME("iface %p, pageWidth %f stub!\n", iface, pageWidth);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_get_PageHeight(ICoreWebView2PrintSettings2 *iface, double *pageHeight)
{
    FIXME("iface %p, pageHeight %p stub!\n", iface, pageHeight);
    if (pageHeight) memset(pageHeight, 0, sizeof(*pageHeight));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_put_PageHeight(ICoreWebView2PrintSettings2 *iface, double pageHeight)
{
    FIXME("iface %p, pageHeight %f stub!\n", iface, pageHeight);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_get_MarginTop(ICoreWebView2PrintSettings2 *iface, double *marginTop)
{
    FIXME("iface %p, marginTop %p stub!\n", iface, marginTop);
    if (marginTop) memset(marginTop, 0, sizeof(*marginTop));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_put_MarginTop(ICoreWebView2PrintSettings2 *iface, double marginTop)
{
    FIXME("iface %p, marginTop %f stub!\n", iface, marginTop);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_get_MarginBottom(ICoreWebView2PrintSettings2 *iface, double *marginBottom)
{
    FIXME("iface %p, marginBottom %p stub!\n", iface, marginBottom);
    if (marginBottom) memset(marginBottom, 0, sizeof(*marginBottom));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_put_MarginBottom(ICoreWebView2PrintSettings2 *iface, double marginBottom)
{
    FIXME("iface %p, marginBottom %f stub!\n", iface, marginBottom);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_get_MarginLeft(ICoreWebView2PrintSettings2 *iface, double *marginLeft)
{
    FIXME("iface %p, marginLeft %p stub!\n", iface, marginLeft);
    if (marginLeft) memset(marginLeft, 0, sizeof(*marginLeft));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_put_MarginLeft(ICoreWebView2PrintSettings2 *iface, double marginLeft)
{
    FIXME("iface %p, marginLeft %f stub!\n", iface, marginLeft);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_get_MarginRight(ICoreWebView2PrintSettings2 *iface, double *marginRight)
{
    FIXME("iface %p, marginRight %p stub!\n", iface, marginRight);
    if (marginRight) memset(marginRight, 0, sizeof(*marginRight));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_put_MarginRight(ICoreWebView2PrintSettings2 *iface, double marginRight)
{
    FIXME("iface %p, marginRight %f stub!\n", iface, marginRight);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_get_ShouldPrintBackgrounds(ICoreWebView2PrintSettings2 *iface, BOOL *shouldPrintBackgrounds)
{
    FIXME("iface %p, shouldPrintBackgrounds %p stub!\n", iface, shouldPrintBackgrounds);
    if (shouldPrintBackgrounds) memset(shouldPrintBackgrounds, 0, sizeof(*shouldPrintBackgrounds));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_put_ShouldPrintBackgrounds(ICoreWebView2PrintSettings2 *iface, BOOL shouldPrintBackgrounds)
{
    FIXME("iface %p, shouldPrintBackgrounds %d stub!\n", iface, (int)shouldPrintBackgrounds);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_get_ShouldPrintSelectionOnly(ICoreWebView2PrintSettings2 *iface, BOOL *shouldPrintSelectionOnly)
{
    FIXME("iface %p, shouldPrintSelectionOnly %p stub!\n", iface, shouldPrintSelectionOnly);
    if (shouldPrintSelectionOnly) memset(shouldPrintSelectionOnly, 0, sizeof(*shouldPrintSelectionOnly));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_put_ShouldPrintSelectionOnly(ICoreWebView2PrintSettings2 *iface, BOOL shouldPrintSelectionOnly)
{
    FIXME("iface %p, shouldPrintSelectionOnly %d stub!\n", iface, (int)shouldPrintSelectionOnly);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_get_ShouldPrintHeaderAndFooter(ICoreWebView2PrintSettings2 *iface, BOOL *shouldPrintHeaderAndFooter)
{
    FIXME("iface %p, shouldPrintHeaderAndFooter %p stub!\n", iface, shouldPrintHeaderAndFooter);
    if (shouldPrintHeaderAndFooter) memset(shouldPrintHeaderAndFooter, 0, sizeof(*shouldPrintHeaderAndFooter));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_put_ShouldPrintHeaderAndFooter(ICoreWebView2PrintSettings2 *iface, BOOL shouldPrintHeaderAndFooter)
{
    FIXME("iface %p, shouldPrintHeaderAndFooter %d stub!\n", iface, (int)shouldPrintHeaderAndFooter);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_get_HeaderTitle(ICoreWebView2PrintSettings2 *iface, LPWSTR *headerTitle)
{
    FIXME("iface %p, headerTitle %p stub!\n", iface, headerTitle);
    if (headerTitle) *headerTitle = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_put_HeaderTitle(ICoreWebView2PrintSettings2 *iface, LPCWSTR headerTitle)
{
    FIXME("iface %p, headerTitle %s stub!\n", iface, debugstr_w(headerTitle));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_get_FooterUri(ICoreWebView2PrintSettings2 *iface, LPWSTR *footerUri)
{
    FIXME("iface %p, footerUri %p stub!\n", iface, footerUri);
    if (footerUri) *footerUri = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings_put_FooterUri(ICoreWebView2PrintSettings2 *iface, LPCWSTR footerUri)
{
    FIXME("iface %p, footerUri %s stub!\n", iface, debugstr_w(footerUri));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings2_get_PageRanges(ICoreWebView2PrintSettings2 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings2_put_PageRanges(ICoreWebView2PrintSettings2 *iface, LPCWSTR value)
{
    FIXME("iface %p, value %s stub!\n", iface, debugstr_w(value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings2_get_PagesPerSide(ICoreWebView2PrintSettings2 *iface, INT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings2_put_PagesPerSide(ICoreWebView2PrintSettings2 *iface, INT32 value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings2_get_Copies(ICoreWebView2PrintSettings2 *iface, INT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings2_put_Copies(ICoreWebView2PrintSettings2 *iface, INT32 value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings2_get_Collation(ICoreWebView2PrintSettings2 *iface, COREWEBVIEW2_PRINT_COLLATION *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings2_put_Collation(ICoreWebView2PrintSettings2 *iface, COREWEBVIEW2_PRINT_COLLATION value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings2_get_ColorMode(ICoreWebView2PrintSettings2 *iface, COREWEBVIEW2_PRINT_COLOR_MODE *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings2_put_ColorMode(ICoreWebView2PrintSettings2 *iface, COREWEBVIEW2_PRINT_COLOR_MODE value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings2_get_Duplex(ICoreWebView2PrintSettings2 *iface, COREWEBVIEW2_PRINT_DUPLEX *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings2_put_Duplex(ICoreWebView2PrintSettings2 *iface, COREWEBVIEW2_PRINT_DUPLEX value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings2_get_MediaSize(ICoreWebView2PrintSettings2 *iface, COREWEBVIEW2_PRINT_MEDIA_SIZE *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings2_put_MediaSize(ICoreWebView2PrintSettings2 *iface, COREWEBVIEW2_PRINT_MEDIA_SIZE value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings2_get_PrinterName(ICoreWebView2PrintSettings2 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PrintSettings2_put_PrinterName(ICoreWebView2PrintSettings2 *iface, LPCWSTR value)
{
    FIXME("iface %p, value %s stub!\n", iface, debugstr_w(value));
    return S_OK;
}

static const ICoreWebView2PrintSettings2Vtbl ICoreWebView2PrintSettings2_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2PrintSettings_get_Orientation,
    ICoreWebView2PrintSettings_put_Orientation,
    ICoreWebView2PrintSettings_get_ScaleFactor,
    ICoreWebView2PrintSettings_put_ScaleFactor,
    ICoreWebView2PrintSettings_get_PageWidth,
    ICoreWebView2PrintSettings_put_PageWidth,
    ICoreWebView2PrintSettings_get_PageHeight,
    ICoreWebView2PrintSettings_put_PageHeight,
    ICoreWebView2PrintSettings_get_MarginTop,
    ICoreWebView2PrintSettings_put_MarginTop,
    ICoreWebView2PrintSettings_get_MarginBottom,
    ICoreWebView2PrintSettings_put_MarginBottom,
    ICoreWebView2PrintSettings_get_MarginLeft,
    ICoreWebView2PrintSettings_put_MarginLeft,
    ICoreWebView2PrintSettings_get_MarginRight,
    ICoreWebView2PrintSettings_put_MarginRight,
    ICoreWebView2PrintSettings_get_ShouldPrintBackgrounds,
    ICoreWebView2PrintSettings_put_ShouldPrintBackgrounds,
    ICoreWebView2PrintSettings_get_ShouldPrintSelectionOnly,
    ICoreWebView2PrintSettings_put_ShouldPrintSelectionOnly,
    ICoreWebView2PrintSettings_get_ShouldPrintHeaderAndFooter,
    ICoreWebView2PrintSettings_put_ShouldPrintHeaderAndFooter,
    ICoreWebView2PrintSettings_get_HeaderTitle,
    ICoreWebView2PrintSettings_put_HeaderTitle,
    ICoreWebView2PrintSettings_get_FooterUri,
    ICoreWebView2PrintSettings_put_FooterUri,
    ICoreWebView2PrintSettings2_get_PageRanges,
    ICoreWebView2PrintSettings2_put_PageRanges,
    ICoreWebView2PrintSettings2_get_PagesPerSide,
    ICoreWebView2PrintSettings2_put_PagesPerSide,
    ICoreWebView2PrintSettings2_get_Copies,
    ICoreWebView2PrintSettings2_put_Copies,
    ICoreWebView2PrintSettings2_get_Collation,
    ICoreWebView2PrintSettings2_put_Collation,
    ICoreWebView2PrintSettings2_get_ColorMode,
    ICoreWebView2PrintSettings2_put_ColorMode,
    ICoreWebView2PrintSettings2_get_Duplex,
    ICoreWebView2PrintSettings2_put_Duplex,
    ICoreWebView2PrintSettings2_get_MediaSize,
    ICoreWebView2PrintSettings2_put_MediaSize,
    ICoreWebView2PrintSettings2_get_PrinterName,
    ICoreWebView2PrintSettings2_put_PrinterName,
};

const struct stub_family stub_family_ICoreWebView2PrintSettings2 =
{
    "ICoreWebView2PrintSettings2",
    &ICoreWebView2PrintSettings2_stub_vtbl,
    ICoreWebView2PrintSettings2_iids,
};

/* ===== ICoreWebView2ProcessExtendedInfo ===== */

static const IID *const ICoreWebView2ProcessExtendedInfo_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ProcessExtendedInfo,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ProcessExtendedInfo_get_ProcessInfo(ICoreWebView2ProcessExtendedInfo *iface, ICoreWebView2ProcessInfo **processInfo)
{
    FIXME("iface %p, processInfo %p stub!\n", iface, processInfo);
    if (processInfo) *processInfo = stub_create(&stub_family_ICoreWebView2ProcessInfo);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ProcessExtendedInfo_get_AssociatedFrameInfos(ICoreWebView2ProcessExtendedInfo *iface, ICoreWebView2FrameInfoCollection **frames)
{
    FIXME("iface %p, frames %p stub!\n", iface, frames);
    if (frames) *frames = stub_create(&stub_family_ICoreWebView2FrameInfoCollection);
    return S_OK;
}

static const ICoreWebView2ProcessExtendedInfoVtbl ICoreWebView2ProcessExtendedInfo_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ProcessExtendedInfo_get_ProcessInfo,
    ICoreWebView2ProcessExtendedInfo_get_AssociatedFrameInfos,
};

const struct stub_family stub_family_ICoreWebView2ProcessExtendedInfo =
{
    "ICoreWebView2ProcessExtendedInfo",
    &ICoreWebView2ProcessExtendedInfo_stub_vtbl,
    ICoreWebView2ProcessExtendedInfo_iids,
};

/* ===== ICoreWebView2ProcessInfo ===== */

static const IID *const ICoreWebView2ProcessInfo_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ProcessInfo,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ProcessInfo_get_ProcessId(ICoreWebView2ProcessInfo *iface, INT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ProcessInfo_get_Kind(ICoreWebView2ProcessInfo *iface, COREWEBVIEW2_PROCESS_KIND *kind)
{
    FIXME("iface %p, kind %p stub!\n", iface, kind);
    if (kind) memset(kind, 0, sizeof(*kind));
    return S_OK;
}

static const ICoreWebView2ProcessInfoVtbl ICoreWebView2ProcessInfo_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ProcessInfo_get_ProcessId,
    ICoreWebView2ProcessInfo_get_Kind,
};

const struct stub_family stub_family_ICoreWebView2ProcessInfo =
{
    "ICoreWebView2ProcessInfo",
    &ICoreWebView2ProcessInfo_stub_vtbl,
    ICoreWebView2ProcessInfo_iids,
};

/* ===== ICoreWebView2ScriptDialogOpeningEventArgs ===== */

static const IID *const ICoreWebView2ScriptDialogOpeningEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ScriptDialogOpeningEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScriptDialogOpeningEventArgs_get_Uri(ICoreWebView2ScriptDialogOpeningEventArgs *iface, LPWSTR *uri)
{
    FIXME("iface %p, uri %p stub!\n", iface, uri);
    if (uri) *uri = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScriptDialogOpeningEventArgs_get_Kind(ICoreWebView2ScriptDialogOpeningEventArgs *iface, COREWEBVIEW2_SCRIPT_DIALOG_KIND *kind)
{
    FIXME("iface %p, kind %p stub!\n", iface, kind);
    if (kind) memset(kind, 0, sizeof(*kind));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScriptDialogOpeningEventArgs_get_Message(ICoreWebView2ScriptDialogOpeningEventArgs *iface, LPWSTR *message)
{
    FIXME("iface %p, message %p stub!\n", iface, message);
    if (message) *message = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScriptDialogOpeningEventArgs_Accept(ICoreWebView2ScriptDialogOpeningEventArgs *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScriptDialogOpeningEventArgs_get_DefaultText(ICoreWebView2ScriptDialogOpeningEventArgs *iface, LPWSTR *defaultText)
{
    FIXME("iface %p, defaultText %p stub!\n", iface, defaultText);
    if (defaultText) *defaultText = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScriptDialogOpeningEventArgs_get_ResultText(ICoreWebView2ScriptDialogOpeningEventArgs *iface, LPWSTR *resultText)
{
    FIXME("iface %p, resultText %p stub!\n", iface, resultText);
    if (resultText) *resultText = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScriptDialogOpeningEventArgs_put_ResultText(ICoreWebView2ScriptDialogOpeningEventArgs *iface, LPCWSTR resultText)
{
    FIXME("iface %p, resultText %s stub!\n", iface, debugstr_w(resultText));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScriptDialogOpeningEventArgs_GetDeferral(ICoreWebView2ScriptDialogOpeningEventArgs *iface, ICoreWebView2Deferral **deferral)
{
    FIXME("iface %p, deferral %p stub!\n", iface, deferral);
    if (deferral) *deferral = stub_create(&stub_family_ICoreWebView2Deferral);
    return S_OK;
}

static const ICoreWebView2ScriptDialogOpeningEventArgsVtbl ICoreWebView2ScriptDialogOpeningEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ScriptDialogOpeningEventArgs_get_Uri,
    ICoreWebView2ScriptDialogOpeningEventArgs_get_Kind,
    ICoreWebView2ScriptDialogOpeningEventArgs_get_Message,
    ICoreWebView2ScriptDialogOpeningEventArgs_Accept,
    ICoreWebView2ScriptDialogOpeningEventArgs_get_DefaultText,
    ICoreWebView2ScriptDialogOpeningEventArgs_get_ResultText,
    ICoreWebView2ScriptDialogOpeningEventArgs_put_ResultText,
    ICoreWebView2ScriptDialogOpeningEventArgs_GetDeferral,
};

const struct stub_family stub_family_ICoreWebView2ScriptDialogOpeningEventArgs =
{
    "ICoreWebView2ScriptDialogOpeningEventArgs",
    &ICoreWebView2ScriptDialogOpeningEventArgs_stub_vtbl,
    ICoreWebView2ScriptDialogOpeningEventArgs_iids,
};

/* ===== ICoreWebView2ScriptException ===== */

static const IID *const ICoreWebView2ScriptException_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ScriptException,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScriptException_get_LineNumber(ICoreWebView2ScriptException *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScriptException_get_ColumnNumber(ICoreWebView2ScriptException *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScriptException_get_Name(ICoreWebView2ScriptException *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScriptException_get_Message(ICoreWebView2ScriptException *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScriptException_get_ToJson(ICoreWebView2ScriptException *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static const ICoreWebView2ScriptExceptionVtbl ICoreWebView2ScriptException_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ScriptException_get_LineNumber,
    ICoreWebView2ScriptException_get_ColumnNumber,
    ICoreWebView2ScriptException_get_Name,
    ICoreWebView2ScriptException_get_Message,
    ICoreWebView2ScriptException_get_ToJson,
};

const struct stub_family stub_family_ICoreWebView2ScriptException =
{
    "ICoreWebView2ScriptException",
    &ICoreWebView2ScriptException_stub_vtbl,
    ICoreWebView2ScriptException_iids,
};

/* ===== ICoreWebView2ServerCertificateErrorDetectedEventArgs ===== */

static const IID *const ICoreWebView2ServerCertificateErrorDetectedEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ServerCertificateErrorDetectedEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServerCertificateErrorDetectedEventArgs_get_ErrorStatus(ICoreWebView2ServerCertificateErrorDetectedEventArgs *iface, COREWEBVIEW2_WEB_ERROR_STATUS *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServerCertificateErrorDetectedEventArgs_get_RequestUri(ICoreWebView2ServerCertificateErrorDetectedEventArgs *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServerCertificateErrorDetectedEventArgs_get_ServerCertificate(ICoreWebView2ServerCertificateErrorDetectedEventArgs *iface, ICoreWebView2Certificate **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2Certificate);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServerCertificateErrorDetectedEventArgs_get_Action(ICoreWebView2ServerCertificateErrorDetectedEventArgs *iface, COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServerCertificateErrorDetectedEventArgs_put_Action(ICoreWebView2ServerCertificateErrorDetectedEventArgs *iface, COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServerCertificateErrorDetectedEventArgs_GetDeferral(ICoreWebView2ServerCertificateErrorDetectedEventArgs *iface, ICoreWebView2Deferral **deferral)
{
    FIXME("iface %p, deferral %p stub!\n", iface, deferral);
    if (deferral) *deferral = stub_create(&stub_family_ICoreWebView2Deferral);
    return S_OK;
}

static const ICoreWebView2ServerCertificateErrorDetectedEventArgsVtbl ICoreWebView2ServerCertificateErrorDetectedEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ServerCertificateErrorDetectedEventArgs_get_ErrorStatus,
    ICoreWebView2ServerCertificateErrorDetectedEventArgs_get_RequestUri,
    ICoreWebView2ServerCertificateErrorDetectedEventArgs_get_ServerCertificate,
    ICoreWebView2ServerCertificateErrorDetectedEventArgs_get_Action,
    ICoreWebView2ServerCertificateErrorDetectedEventArgs_put_Action,
    ICoreWebView2ServerCertificateErrorDetectedEventArgs_GetDeferral,
};

const struct stub_family stub_family_ICoreWebView2ServerCertificateErrorDetectedEventArgs =
{
    "ICoreWebView2ServerCertificateErrorDetectedEventArgs",
    &ICoreWebView2ServerCertificateErrorDetectedEventArgs_stub_vtbl,
    ICoreWebView2ServerCertificateErrorDetectedEventArgs_iids,
};

/* ===== ICoreWebView2SharedBuffer ===== */

static const IID *const ICoreWebView2SharedBuffer_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2SharedBuffer,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2SharedBuffer_get_Size(ICoreWebView2SharedBuffer *iface, UINT64 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SharedBuffer_get_Buffer(ICoreWebView2SharedBuffer *iface, BYTE **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = NULL;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SharedBuffer_OpenStream(ICoreWebView2SharedBuffer *iface, IStream **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = NULL;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SharedBuffer_get_FileMappingHandle(ICoreWebView2SharedBuffer *iface, HANDLE *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SharedBuffer_Close(ICoreWebView2SharedBuffer *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static const ICoreWebView2SharedBufferVtbl ICoreWebView2SharedBuffer_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2SharedBuffer_get_Size,
    ICoreWebView2SharedBuffer_get_Buffer,
    ICoreWebView2SharedBuffer_OpenStream,
    ICoreWebView2SharedBuffer_get_FileMappingHandle,
    ICoreWebView2SharedBuffer_Close,
};

const struct stub_family stub_family_ICoreWebView2SharedBuffer =
{
    "ICoreWebView2SharedBuffer",
    &ICoreWebView2SharedBuffer_stub_vtbl,
    ICoreWebView2SharedBuffer_iids,
};

/* ===== ICoreWebView2WebResourceRequest ===== */

static const IID *const ICoreWebView2WebResourceRequest_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2WebResourceRequest,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceRequest_get_Uri(ICoreWebView2WebResourceRequest *iface, LPWSTR *uri)
{
    FIXME("iface %p, uri %p stub!\n", iface, uri);
    if (uri) *uri = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceRequest_put_Uri(ICoreWebView2WebResourceRequest *iface, LPCWSTR uri)
{
    FIXME("iface %p, uri %s stub!\n", iface, debugstr_w(uri));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceRequest_get_Method(ICoreWebView2WebResourceRequest *iface, LPWSTR *method)
{
    FIXME("iface %p, method %p stub!\n", iface, method);
    if (method) *method = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceRequest_put_Method(ICoreWebView2WebResourceRequest *iface, LPCWSTR method)
{
    FIXME("iface %p, method %s stub!\n", iface, debugstr_w(method));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceRequest_get_Content(ICoreWebView2WebResourceRequest *iface, IStream **content)
{
    FIXME("iface %p, content %p stub!\n", iface, content);
    if (content) *content = NULL;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceRequest_put_Content(ICoreWebView2WebResourceRequest *iface, IStream *content)
{
    FIXME("iface %p, content %p stub!\n", iface, (void *)content);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceRequest_get_Headers(ICoreWebView2WebResourceRequest *iface, ICoreWebView2HttpRequestHeaders **headers)
{
    FIXME("iface %p, headers %p stub!\n", iface, headers);
    if (headers) *headers = stub_create(&stub_family_ICoreWebView2HttpRequestHeaders);
    return S_OK;
}

static const ICoreWebView2WebResourceRequestVtbl ICoreWebView2WebResourceRequest_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2WebResourceRequest_get_Uri,
    ICoreWebView2WebResourceRequest_put_Uri,
    ICoreWebView2WebResourceRequest_get_Method,
    ICoreWebView2WebResourceRequest_put_Method,
    ICoreWebView2WebResourceRequest_get_Content,
    ICoreWebView2WebResourceRequest_put_Content,
    ICoreWebView2WebResourceRequest_get_Headers,
};

const struct stub_family stub_family_ICoreWebView2WebResourceRequest =
{
    "ICoreWebView2WebResourceRequest",
    &ICoreWebView2WebResourceRequest_stub_vtbl,
    ICoreWebView2WebResourceRequest_iids,
};

/* ===== ICoreWebView2WebResourceResponse ===== */

static const IID *const ICoreWebView2WebResourceResponse_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2WebResourceResponse,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceResponse_get_Content(ICoreWebView2WebResourceResponse *iface, IStream **content)
{
    FIXME("iface %p, content %p stub!\n", iface, content);
    if (content) *content = NULL;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceResponse_put_Content(ICoreWebView2WebResourceResponse *iface, IStream *content)
{
    FIXME("iface %p, content %p stub!\n", iface, (void *)content);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceResponse_get_Headers(ICoreWebView2WebResourceResponse *iface, ICoreWebView2HttpResponseHeaders **headers)
{
    FIXME("iface %p, headers %p stub!\n", iface, headers);
    if (headers) *headers = stub_create(&stub_family_ICoreWebView2HttpResponseHeaders);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceResponse_get_StatusCode(ICoreWebView2WebResourceResponse *iface, int *statusCode)
{
    FIXME("iface %p, statusCode %p stub!\n", iface, statusCode);
    if (statusCode) memset(statusCode, 0, sizeof(*statusCode));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceResponse_put_StatusCode(ICoreWebView2WebResourceResponse *iface, int statusCode)
{
    FIXME("iface %p, statusCode %d stub!\n", iface, (int)statusCode);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceResponse_get_ReasonPhrase(ICoreWebView2WebResourceResponse *iface, LPWSTR *reasonPhrase)
{
    FIXME("iface %p, reasonPhrase %p stub!\n", iface, reasonPhrase);
    if (reasonPhrase) *reasonPhrase = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceResponse_put_ReasonPhrase(ICoreWebView2WebResourceResponse *iface, LPCWSTR reasonPhrase)
{
    FIXME("iface %p, reasonPhrase %s stub!\n", iface, debugstr_w(reasonPhrase));
    return S_OK;
}

static const ICoreWebView2WebResourceResponseVtbl ICoreWebView2WebResourceResponse_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2WebResourceResponse_get_Content,
    ICoreWebView2WebResourceResponse_put_Content,
    ICoreWebView2WebResourceResponse_get_Headers,
    ICoreWebView2WebResourceResponse_get_StatusCode,
    ICoreWebView2WebResourceResponse_put_StatusCode,
    ICoreWebView2WebResourceResponse_get_ReasonPhrase,
    ICoreWebView2WebResourceResponse_put_ReasonPhrase,
};

const struct stub_family stub_family_ICoreWebView2WebResourceResponse =
{
    "ICoreWebView2WebResourceResponse",
    &ICoreWebView2WebResourceResponse_stub_vtbl,
    ICoreWebView2WebResourceResponse_iids,
};

/* ===== ICoreWebView2WebResourceResponseView ===== */

static const IID *const ICoreWebView2WebResourceResponseView_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2WebResourceResponseView,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceResponseView_get_Headers(ICoreWebView2WebResourceResponseView *iface, ICoreWebView2HttpResponseHeaders **headers)
{
    FIXME("iface %p, headers %p stub!\n", iface, headers);
    if (headers) *headers = stub_create(&stub_family_ICoreWebView2HttpResponseHeaders);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceResponseView_get_StatusCode(ICoreWebView2WebResourceResponseView *iface, int *statusCode)
{
    FIXME("iface %p, statusCode %p stub!\n", iface, statusCode);
    if (statusCode) memset(statusCode, 0, sizeof(*statusCode));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceResponseView_get_ReasonPhrase(ICoreWebView2WebResourceResponseView *iface, LPWSTR *reasonPhrase)
{
    FIXME("iface %p, reasonPhrase %p stub!\n", iface, reasonPhrase);
    if (reasonPhrase) *reasonPhrase = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceResponseView_GetContent(ICoreWebView2WebResourceResponseView *iface, ICoreWebView2WebResourceResponseViewGetContentCompletedHandler *handler)
{
    FIXME("iface %p, handler %p stub!\n", iface, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2WebResourceResponseViewGetContentCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static const ICoreWebView2WebResourceResponseViewVtbl ICoreWebView2WebResourceResponseView_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2WebResourceResponseView_get_Headers,
    ICoreWebView2WebResourceResponseView_get_StatusCode,
    ICoreWebView2WebResourceResponseView_get_ReasonPhrase,
    ICoreWebView2WebResourceResponseView_GetContent,
};

const struct stub_family stub_family_ICoreWebView2WebResourceResponseView =
{
    "ICoreWebView2WebResourceResponseView",
    &ICoreWebView2WebResourceResponseView_stub_vtbl,
    ICoreWebView2WebResourceResponseView_iids,
};

/* ===== ICoreWebView2WindowFeatures ===== */

static const IID *const ICoreWebView2WindowFeatures_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2WindowFeatures,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2WindowFeatures_get_HasPosition(ICoreWebView2WindowFeatures *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WindowFeatures_get_HasSize(ICoreWebView2WindowFeatures *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WindowFeatures_get_Left(ICoreWebView2WindowFeatures *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WindowFeatures_get_Top(ICoreWebView2WindowFeatures *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WindowFeatures_get_Height(ICoreWebView2WindowFeatures *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WindowFeatures_get_Width(ICoreWebView2WindowFeatures *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WindowFeatures_get_ShouldDisplayMenuBar(ICoreWebView2WindowFeatures *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WindowFeatures_get_ShouldDisplayStatus(ICoreWebView2WindowFeatures *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WindowFeatures_get_ShouldDisplayToolbar(ICoreWebView2WindowFeatures *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WindowFeatures_get_ShouldDisplayScrollBars(ICoreWebView2WindowFeatures *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2WindowFeaturesVtbl ICoreWebView2WindowFeatures_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2WindowFeatures_get_HasPosition,
    ICoreWebView2WindowFeatures_get_HasSize,
    ICoreWebView2WindowFeatures_get_Left,
    ICoreWebView2WindowFeatures_get_Top,
    ICoreWebView2WindowFeatures_get_Height,
    ICoreWebView2WindowFeatures_get_Width,
    ICoreWebView2WindowFeatures_get_ShouldDisplayMenuBar,
    ICoreWebView2WindowFeatures_get_ShouldDisplayStatus,
    ICoreWebView2WindowFeatures_get_ShouldDisplayToolbar,
    ICoreWebView2WindowFeatures_get_ShouldDisplayScrollBars,
};

const struct stub_family stub_family_ICoreWebView2WindowFeatures =
{
    "ICoreWebView2WindowFeatures",
    &ICoreWebView2WindowFeatures_stub_vtbl,
    ICoreWebView2WindowFeatures_iids,
};

/* ===== ICoreWebView2ObjectCollection ===== */

static const IID *const ICoreWebView2ObjectCollection_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ObjectCollectionView,
    &IID_ICoreWebView2ObjectCollection,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ObjectCollectionView_get_Count(ICoreWebView2ObjectCollection *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ObjectCollectionView_GetValueAtIndex(ICoreWebView2ObjectCollection *iface, UINT32 index, IUnknown **value)
{
    FIXME("iface %p, index %d, value %p stub!\n", iface, (int)index, value);
    if (value) *value = NULL;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ObjectCollection_RemoveValueAtIndex(ICoreWebView2ObjectCollection *iface, UINT32 index)
{
    FIXME("iface %p, index %d stub!\n", iface, (int)index);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ObjectCollection_InsertValueAtIndex(ICoreWebView2ObjectCollection *iface, UINT32 index, IUnknown *value)
{
    FIXME("iface %p, index %d, value %p stub!\n", iface, (int)index, (void *)value);
    return S_OK;
}

static const ICoreWebView2ObjectCollectionVtbl ICoreWebView2ObjectCollection_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ObjectCollectionView_get_Count,
    ICoreWebView2ObjectCollectionView_GetValueAtIndex,
    ICoreWebView2ObjectCollection_RemoveValueAtIndex,
    ICoreWebView2ObjectCollection_InsertValueAtIndex,
};

const struct stub_family stub_family_ICoreWebView2ObjectCollection =
{
    "ICoreWebView2ObjectCollection",
    &ICoreWebView2ObjectCollection_stub_vtbl,
    ICoreWebView2ObjectCollection_iids,
};

/* ===== ICoreWebView2Notification ===== */

static const IID *const ICoreWebView2Notification_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2Notification,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_add_CloseRequested(ICoreWebView2Notification *iface, ICoreWebView2NotificationCloseRequestedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_remove_CloseRequested(ICoreWebView2Notification *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_ReportShown(ICoreWebView2Notification *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_ReportClicked(ICoreWebView2Notification *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_ReportClosed(ICoreWebView2Notification *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_get_Body(ICoreWebView2Notification *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_get_Direction(ICoreWebView2Notification *iface, COREWEBVIEW2_TEXT_DIRECTION_KIND *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_get_Language(ICoreWebView2Notification *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_get_Tag(ICoreWebView2Notification *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_get_IconUri(ICoreWebView2Notification *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_get_Title(ICoreWebView2Notification *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_get_BadgeUri(ICoreWebView2Notification *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_get_BodyImageUri(ICoreWebView2Notification *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_get_ShouldRenotify(ICoreWebView2Notification *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_get_RequiresInteraction(ICoreWebView2Notification *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_get_IsSilent(ICoreWebView2Notification *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_get_Timestamp(ICoreWebView2Notification *iface, double *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Notification_GetVibrationPattern(ICoreWebView2Notification *iface, UINT32 *count, UINT64 **vibrationPattern)
{
    FIXME("iface %p, count %p, vibrationPattern %p stub!\n", iface, count, vibrationPattern);
    if (count) memset(count, 0, sizeof(*count));
    if (vibrationPattern) *vibrationPattern = NULL;
    return S_OK;
}

static const ICoreWebView2NotificationVtbl ICoreWebView2Notification_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2Notification_add_CloseRequested,
    ICoreWebView2Notification_remove_CloseRequested,
    ICoreWebView2Notification_ReportShown,
    ICoreWebView2Notification_ReportClicked,
    ICoreWebView2Notification_ReportClosed,
    ICoreWebView2Notification_get_Body,
    ICoreWebView2Notification_get_Direction,
    ICoreWebView2Notification_get_Language,
    ICoreWebView2Notification_get_Tag,
    ICoreWebView2Notification_get_IconUri,
    ICoreWebView2Notification_get_Title,
    ICoreWebView2Notification_get_BadgeUri,
    ICoreWebView2Notification_get_BodyImageUri,
    ICoreWebView2Notification_get_ShouldRenotify,
    ICoreWebView2Notification_get_RequiresInteraction,
    ICoreWebView2Notification_get_IsSilent,
    ICoreWebView2Notification_get_Timestamp,
    ICoreWebView2Notification_GetVibrationPattern,
};

const struct stub_family stub_family_ICoreWebView2Notification =
{
    "ICoreWebView2Notification",
    &ICoreWebView2Notification_stub_vtbl,
    ICoreWebView2Notification_iids,
};

/* ===== ICoreWebView2NotificationReceivedEventArgs ===== */

static const IID *const ICoreWebView2NotificationReceivedEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2NotificationReceivedEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2NotificationReceivedEventArgs_get_SenderOrigin(ICoreWebView2NotificationReceivedEventArgs *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NotificationReceivedEventArgs_get_Notification(ICoreWebView2NotificationReceivedEventArgs *iface, ICoreWebView2Notification **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2Notification);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NotificationReceivedEventArgs_put_Handled(ICoreWebView2NotificationReceivedEventArgs *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NotificationReceivedEventArgs_get_Handled(ICoreWebView2NotificationReceivedEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NotificationReceivedEventArgs_GetDeferral(ICoreWebView2NotificationReceivedEventArgs *iface, ICoreWebView2Deferral **deferral)
{
    FIXME("iface %p, deferral %p stub!\n", iface, deferral);
    if (deferral) *deferral = stub_create(&stub_family_ICoreWebView2Deferral);
    return S_OK;
}

static const ICoreWebView2NotificationReceivedEventArgsVtbl ICoreWebView2NotificationReceivedEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2NotificationReceivedEventArgs_get_SenderOrigin,
    ICoreWebView2NotificationReceivedEventArgs_get_Notification,
    ICoreWebView2NotificationReceivedEventArgs_put_Handled,
    ICoreWebView2NotificationReceivedEventArgs_get_Handled,
    ICoreWebView2NotificationReceivedEventArgs_GetDeferral,
};

const struct stub_family stub_family_ICoreWebView2NotificationReceivedEventArgs =
{
    "ICoreWebView2NotificationReceivedEventArgs",
    &ICoreWebView2NotificationReceivedEventArgs_stub_vtbl,
    ICoreWebView2NotificationReceivedEventArgs_iids,
};

/* ===== ICoreWebView2SaveAsUIShowingEventArgs ===== */

static const IID *const ICoreWebView2SaveAsUIShowingEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2SaveAsUIShowingEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveAsUIShowingEventArgs_get_ContentMimeType(ICoreWebView2SaveAsUIShowingEventArgs *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveAsUIShowingEventArgs_put_Cancel(ICoreWebView2SaveAsUIShowingEventArgs *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveAsUIShowingEventArgs_get_Cancel(ICoreWebView2SaveAsUIShowingEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveAsUIShowingEventArgs_put_SuppressDefaultDialog(ICoreWebView2SaveAsUIShowingEventArgs *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveAsUIShowingEventArgs_get_SuppressDefaultDialog(ICoreWebView2SaveAsUIShowingEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveAsUIShowingEventArgs_GetDeferral(ICoreWebView2SaveAsUIShowingEventArgs *iface, ICoreWebView2Deferral **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2Deferral);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveAsUIShowingEventArgs_put_SaveAsFilePath(ICoreWebView2SaveAsUIShowingEventArgs *iface, LPCWSTR value)
{
    FIXME("iface %p, value %s stub!\n", iface, debugstr_w(value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveAsUIShowingEventArgs_get_SaveAsFilePath(ICoreWebView2SaveAsUIShowingEventArgs *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveAsUIShowingEventArgs_put_AllowReplace(ICoreWebView2SaveAsUIShowingEventArgs *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveAsUIShowingEventArgs_get_AllowReplace(ICoreWebView2SaveAsUIShowingEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveAsUIShowingEventArgs_put_Kind(ICoreWebView2SaveAsUIShowingEventArgs *iface, COREWEBVIEW2_SAVE_AS_KIND value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveAsUIShowingEventArgs_get_Kind(ICoreWebView2SaveAsUIShowingEventArgs *iface, COREWEBVIEW2_SAVE_AS_KIND *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2SaveAsUIShowingEventArgsVtbl ICoreWebView2SaveAsUIShowingEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2SaveAsUIShowingEventArgs_get_ContentMimeType,
    ICoreWebView2SaveAsUIShowingEventArgs_put_Cancel,
    ICoreWebView2SaveAsUIShowingEventArgs_get_Cancel,
    ICoreWebView2SaveAsUIShowingEventArgs_put_SuppressDefaultDialog,
    ICoreWebView2SaveAsUIShowingEventArgs_get_SuppressDefaultDialog,
    ICoreWebView2SaveAsUIShowingEventArgs_GetDeferral,
    ICoreWebView2SaveAsUIShowingEventArgs_put_SaveAsFilePath,
    ICoreWebView2SaveAsUIShowingEventArgs_get_SaveAsFilePath,
    ICoreWebView2SaveAsUIShowingEventArgs_put_AllowReplace,
    ICoreWebView2SaveAsUIShowingEventArgs_get_AllowReplace,
    ICoreWebView2SaveAsUIShowingEventArgs_put_Kind,
    ICoreWebView2SaveAsUIShowingEventArgs_get_Kind,
};

const struct stub_family stub_family_ICoreWebView2SaveAsUIShowingEventArgs =
{
    "ICoreWebView2SaveAsUIShowingEventArgs",
    &ICoreWebView2SaveAsUIShowingEventArgs_stub_vtbl,
    ICoreWebView2SaveAsUIShowingEventArgs_iids,
};

/* ===== ICoreWebView2AcceleratorKeyPressedEventArgs2 ===== */

static const IID *const ICoreWebView2AcceleratorKeyPressedEventArgs2_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2AcceleratorKeyPressedEventArgs,
    &IID_ICoreWebView2AcceleratorKeyPressedEventArgs2,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2AcceleratorKeyPressedEventArgs_get_KeyEventKind(ICoreWebView2AcceleratorKeyPressedEventArgs2 *iface, COREWEBVIEW2_KEY_EVENT_KIND *keyEventKind)
{
    FIXME("iface %p, keyEventKind %p stub!\n", iface, keyEventKind);
    if (keyEventKind) memset(keyEventKind, 0, sizeof(*keyEventKind));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2AcceleratorKeyPressedEventArgs_get_VirtualKey(ICoreWebView2AcceleratorKeyPressedEventArgs2 *iface, UINT *virtualKey)
{
    FIXME("iface %p, virtualKey %p stub!\n", iface, virtualKey);
    if (virtualKey) memset(virtualKey, 0, sizeof(*virtualKey));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2AcceleratorKeyPressedEventArgs_get_KeyEventLParam(ICoreWebView2AcceleratorKeyPressedEventArgs2 *iface, INT *lParam)
{
    FIXME("iface %p, lParam %p stub!\n", iface, lParam);
    if (lParam) memset(lParam, 0, sizeof(*lParam));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2AcceleratorKeyPressedEventArgs_get_PhysicalKeyStatus(ICoreWebView2AcceleratorKeyPressedEventArgs2 *iface, COREWEBVIEW2_PHYSICAL_KEY_STATUS *physicalKeyStatus)
{
    FIXME("iface %p, physicalKeyStatus %p stub!\n", iface, physicalKeyStatus);
    if (physicalKeyStatus) memset(physicalKeyStatus, 0, sizeof(*physicalKeyStatus));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2AcceleratorKeyPressedEventArgs_get_Handled(ICoreWebView2AcceleratorKeyPressedEventArgs2 *iface, BOOL *handled)
{
    FIXME("iface %p, handled %p stub!\n", iface, handled);
    if (handled) memset(handled, 0, sizeof(*handled));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2AcceleratorKeyPressedEventArgs_put_Handled(ICoreWebView2AcceleratorKeyPressedEventArgs2 *iface, BOOL handled)
{
    FIXME("iface %p, handled %d stub!\n", iface, (int)handled);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2AcceleratorKeyPressedEventArgs2_get_IsBrowserAcceleratorKeyEnabled(ICoreWebView2AcceleratorKeyPressedEventArgs2 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2AcceleratorKeyPressedEventArgs2_put_IsBrowserAcceleratorKeyEnabled(ICoreWebView2AcceleratorKeyPressedEventArgs2 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static const ICoreWebView2AcceleratorKeyPressedEventArgs2Vtbl ICoreWebView2AcceleratorKeyPressedEventArgs2_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2AcceleratorKeyPressedEventArgs_get_KeyEventKind,
    ICoreWebView2AcceleratorKeyPressedEventArgs_get_VirtualKey,
    ICoreWebView2AcceleratorKeyPressedEventArgs_get_KeyEventLParam,
    ICoreWebView2AcceleratorKeyPressedEventArgs_get_PhysicalKeyStatus,
    ICoreWebView2AcceleratorKeyPressedEventArgs_get_Handled,
    ICoreWebView2AcceleratorKeyPressedEventArgs_put_Handled,
    ICoreWebView2AcceleratorKeyPressedEventArgs2_get_IsBrowserAcceleratorKeyEnabled,
    ICoreWebView2AcceleratorKeyPressedEventArgs2_put_IsBrowserAcceleratorKeyEnabled,
};

const struct stub_family stub_family_ICoreWebView2AcceleratorKeyPressedEventArgs2 =
{
    "ICoreWebView2AcceleratorKeyPressedEventArgs2",
    &ICoreWebView2AcceleratorKeyPressedEventArgs2_stub_vtbl,
    ICoreWebView2AcceleratorKeyPressedEventArgs2_iids,
};

/* ===== ICoreWebView2BrowserProcessExitedEventArgs ===== */

static const IID *const ICoreWebView2BrowserProcessExitedEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2BrowserProcessExitedEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2BrowserProcessExitedEventArgs_get_BrowserProcessExitKind(ICoreWebView2BrowserProcessExitedEventArgs *iface, COREWEBVIEW2_BROWSER_PROCESS_EXIT_KIND *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2BrowserProcessExitedEventArgs_get_BrowserProcessId(ICoreWebView2BrowserProcessExitedEventArgs *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2BrowserProcessExitedEventArgsVtbl ICoreWebView2BrowserProcessExitedEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2BrowserProcessExitedEventArgs_get_BrowserProcessExitKind,
    ICoreWebView2BrowserProcessExitedEventArgs_get_BrowserProcessId,
};

const struct stub_family stub_family_ICoreWebView2BrowserProcessExitedEventArgs =
{
    "ICoreWebView2BrowserProcessExitedEventArgs",
    &ICoreWebView2BrowserProcessExitedEventArgs_stub_vtbl,
    ICoreWebView2BrowserProcessExitedEventArgs_iids,
};

/* ===== ICoreWebView2CompositionController5 ===== */

static const IID *const ICoreWebView2CompositionController5_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2CompositionController,
    &IID_ICoreWebView2CompositionController2,
    &IID_ICoreWebView2CompositionController3,
    &IID_ICoreWebView2CompositionController4,
    &IID_ICoreWebView2CompositionController5,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController_get_RootVisualTarget(ICoreWebView2CompositionController5 *iface, IUnknown **target)
{
    FIXME("iface %p, target %p stub!\n", iface, target);
    if (target) *target = NULL;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController_put_RootVisualTarget(ICoreWebView2CompositionController5 *iface, IUnknown *target)
{
    FIXME("iface %p, target %p stub!\n", iface, (void *)target);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController_SendMouseInput(ICoreWebView2CompositionController5 *iface, COREWEBVIEW2_MOUSE_EVENT_KIND eventKind, COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS virtualKeys, UINT32 mouseData, POINT point)
{
    FIXME("iface %p, eventKind %d, virtualKeys %d, mouseData %d, point %s stub!\n", iface, (int)eventKind, (int)virtualKeys, (int)mouseData, wine_dbgstr_point(&point));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController_SendPointerInput(ICoreWebView2CompositionController5 *iface, COREWEBVIEW2_POINTER_EVENT_KIND eventKind, ICoreWebView2PointerInfo *pointerInfo)
{
    FIXME("iface %p, eventKind %d, pointerInfo %p stub!\n", iface, (int)eventKind, (void *)pointerInfo);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController_get_Cursor(ICoreWebView2CompositionController5 *iface, HCURSOR *cursor)
{
    FIXME("iface %p, cursor %p stub!\n", iface, cursor);
    if (cursor) memset(cursor, 0, sizeof(*cursor));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController_get_SystemCursorId(ICoreWebView2CompositionController5 *iface, UINT32 *systemCursorId)
{
    FIXME("iface %p, systemCursorId %p stub!\n", iface, systemCursorId);
    if (systemCursorId) memset(systemCursorId, 0, sizeof(*systemCursorId));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController_add_CursorChanged(ICoreWebView2CompositionController5 *iface, ICoreWebView2CursorChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController_remove_CursorChanged(ICoreWebView2CompositionController5 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController2_get_AutomationProvider(ICoreWebView2CompositionController5 *iface, IUnknown **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = NULL;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController3_DragEnter(ICoreWebView2CompositionController5 *iface, IDataObject *dataObject, DWORD keyState, POINT point, DWORD *effect)
{
    FIXME("iface %p, dataObject %p, keyState %d, point %s, effect %p stub!\n", iface, (void *)dataObject, (int)keyState, wine_dbgstr_point(&point), effect);
    if (effect) memset(effect, 0, sizeof(*effect));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController3_DragLeave(ICoreWebView2CompositionController5 *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController3_DragOver(ICoreWebView2CompositionController5 *iface, DWORD keyState, POINT point, DWORD *effect)
{
    FIXME("iface %p, keyState %d, point %s, effect %p stub!\n", iface, (int)keyState, wine_dbgstr_point(&point), effect);
    if (effect) memset(effect, 0, sizeof(*effect));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController3_Drop(ICoreWebView2CompositionController5 *iface, IDataObject *dataObject, DWORD keyState, POINT point, DWORD *effect)
{
    FIXME("iface %p, dataObject %p, keyState %d, point %s, effect %p stub!\n", iface, (void *)dataObject, (int)keyState, wine_dbgstr_point(&point), effect);
    if (effect) memset(effect, 0, sizeof(*effect));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController4_GetNonClientRegionAtPoint(ICoreWebView2CompositionController5 *iface, POINT point, COREWEBVIEW2_NON_CLIENT_REGION_KIND *value)
{
    FIXME("iface %p, point %s, value %p stub!\n", iface, wine_dbgstr_point(&point), value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController4_QueryNonClientRegion(ICoreWebView2CompositionController5 *iface, COREWEBVIEW2_NON_CLIENT_REGION_KIND kind, ICoreWebView2RegionRectCollectionView **rects)
{
    FIXME("iface %p, kind %d, rects %p stub!\n", iface, (int)kind, rects);
    if (rects) *rects = stub_create(&stub_family_ICoreWebView2RegionRectCollectionView);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController4_add_NonClientRegionChanged(ICoreWebView2CompositionController5 *iface, ICoreWebView2NonClientRegionChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController4_remove_NonClientRegionChanged(ICoreWebView2CompositionController5 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController5_add_DragStarting(ICoreWebView2CompositionController5 *iface, ICoreWebView2DragStartingEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CompositionController5_remove_DragStarting(ICoreWebView2CompositionController5 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static const ICoreWebView2CompositionController5Vtbl ICoreWebView2CompositionController5_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2CompositionController_get_RootVisualTarget,
    ICoreWebView2CompositionController_put_RootVisualTarget,
    ICoreWebView2CompositionController_SendMouseInput,
    ICoreWebView2CompositionController_SendPointerInput,
    ICoreWebView2CompositionController_get_Cursor,
    ICoreWebView2CompositionController_get_SystemCursorId,
    ICoreWebView2CompositionController_add_CursorChanged,
    ICoreWebView2CompositionController_remove_CursorChanged,
    ICoreWebView2CompositionController2_get_AutomationProvider,
    ICoreWebView2CompositionController3_DragEnter,
    ICoreWebView2CompositionController3_DragLeave,
    ICoreWebView2CompositionController3_DragOver,
    ICoreWebView2CompositionController3_Drop,
    ICoreWebView2CompositionController4_GetNonClientRegionAtPoint,
    ICoreWebView2CompositionController4_QueryNonClientRegion,
    ICoreWebView2CompositionController4_add_NonClientRegionChanged,
    ICoreWebView2CompositionController4_remove_NonClientRegionChanged,
    ICoreWebView2CompositionController5_add_DragStarting,
    ICoreWebView2CompositionController5_remove_DragStarting,
};

const struct stub_family stub_family_ICoreWebView2CompositionController5 =
{
    "ICoreWebView2CompositionController5",
    &ICoreWebView2CompositionController5_stub_vtbl,
    ICoreWebView2CompositionController5_iids,
};

/* ===== ICoreWebView2ContentLoadingEventArgs ===== */

static const IID *const ICoreWebView2ContentLoadingEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ContentLoadingEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContentLoadingEventArgs_get_IsErrorPage(ICoreWebView2ContentLoadingEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContentLoadingEventArgs_get_NavigationId(ICoreWebView2ContentLoadingEventArgs *iface, UINT64 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2ContentLoadingEventArgsVtbl ICoreWebView2ContentLoadingEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ContentLoadingEventArgs_get_IsErrorPage,
    ICoreWebView2ContentLoadingEventArgs_get_NavigationId,
};

const struct stub_family stub_family_ICoreWebView2ContentLoadingEventArgs =
{
    "ICoreWebView2ContentLoadingEventArgs",
    &ICoreWebView2ContentLoadingEventArgs_stub_vtbl,
    ICoreWebView2ContentLoadingEventArgs_iids,
};

/* ===== ICoreWebView2ContextMenuItemCollection ===== */

static const IID *const ICoreWebView2ContextMenuItemCollection_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ContextMenuItemCollection,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItemCollection_get_Count(ICoreWebView2ContextMenuItemCollection *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItemCollection_GetValueAtIndex(ICoreWebView2ContextMenuItemCollection *iface, UINT32 index, ICoreWebView2ContextMenuItem **value)
{
    FIXME("iface %p, index %d, value %p stub!\n", iface, (int)index, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ContextMenuItem);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItemCollection_RemoveValueAtIndex(ICoreWebView2ContextMenuItemCollection *iface, UINT32 index)
{
    FIXME("iface %p, index %d stub!\n", iface, (int)index);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ContextMenuItemCollection_InsertValueAtIndex(ICoreWebView2ContextMenuItemCollection *iface, UINT32 index, ICoreWebView2ContextMenuItem *value)
{
    FIXME("iface %p, index %d, value %p stub!\n", iface, (int)index, (void *)value);
    return S_OK;
}

static const ICoreWebView2ContextMenuItemCollectionVtbl ICoreWebView2ContextMenuItemCollection_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ContextMenuItemCollection_get_Count,
    ICoreWebView2ContextMenuItemCollection_GetValueAtIndex,
    ICoreWebView2ContextMenuItemCollection_RemoveValueAtIndex,
    ICoreWebView2ContextMenuItemCollection_InsertValueAtIndex,
};

const struct stub_family stub_family_ICoreWebView2ContextMenuItemCollection =
{
    "ICoreWebView2ContextMenuItemCollection",
    &ICoreWebView2ContextMenuItemCollection_stub_vtbl,
    ICoreWebView2ContextMenuItemCollection_iids,
};

/* ===== ICoreWebView2Controller4 ===== */

static const IID *const ICoreWebView2Controller4_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2Controller,
    &IID_ICoreWebView2Controller2,
    &IID_ICoreWebView2Controller3,
    &IID_ICoreWebView2Controller4,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_get_IsVisible(ICoreWebView2Controller4 *iface, BOOL *isVisible)
{
    FIXME("iface %p, isVisible %p stub!\n", iface, isVisible);
    if (isVisible) memset(isVisible, 0, sizeof(*isVisible));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_put_IsVisible(ICoreWebView2Controller4 *iface, BOOL isVisible)
{
    FIXME("iface %p, isVisible %d stub!\n", iface, (int)isVisible);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_get_Bounds(ICoreWebView2Controller4 *iface, RECT *bounds)
{
    FIXME("iface %p, bounds %p stub!\n", iface, bounds);
    if (bounds) memset(bounds, 0, sizeof(*bounds));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_put_Bounds(ICoreWebView2Controller4 *iface, RECT bounds)
{
    FIXME("iface %p, bounds %s stub!\n", iface, wine_dbgstr_rect(&bounds));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_get_ZoomFactor(ICoreWebView2Controller4 *iface, double *zoomFactor)
{
    FIXME("iface %p, zoomFactor %p stub!\n", iface, zoomFactor);
    if (zoomFactor) memset(zoomFactor, 0, sizeof(*zoomFactor));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_put_ZoomFactor(ICoreWebView2Controller4 *iface, double zoomFactor)
{
    FIXME("iface %p, zoomFactor %f stub!\n", iface, zoomFactor);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_add_ZoomFactorChanged(ICoreWebView2Controller4 *iface, ICoreWebView2ZoomFactorChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_remove_ZoomFactorChanged(ICoreWebView2Controller4 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_SetBoundsAndZoomFactor(ICoreWebView2Controller4 *iface, RECT bounds, double zoomFactor)
{
    FIXME("iface %p, bounds %s, zoomFactor %f stub!\n", iface, wine_dbgstr_rect(&bounds), zoomFactor);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_MoveFocus(ICoreWebView2Controller4 *iface, COREWEBVIEW2_MOVE_FOCUS_REASON reason)
{
    FIXME("iface %p, reason %d stub!\n", iface, (int)reason);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_add_MoveFocusRequested(ICoreWebView2Controller4 *iface, ICoreWebView2MoveFocusRequestedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_remove_MoveFocusRequested(ICoreWebView2Controller4 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_add_GotFocus(ICoreWebView2Controller4 *iface, ICoreWebView2FocusChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_remove_GotFocus(ICoreWebView2Controller4 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_add_LostFocus(ICoreWebView2Controller4 *iface, ICoreWebView2FocusChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_remove_LostFocus(ICoreWebView2Controller4 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_add_AcceleratorKeyPressed(ICoreWebView2Controller4 *iface, ICoreWebView2AcceleratorKeyPressedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_remove_AcceleratorKeyPressed(ICoreWebView2Controller4 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_get_ParentWindow(ICoreWebView2Controller4 *iface, HWND *parentWindow)
{
    FIXME("iface %p, parentWindow %p stub!\n", iface, parentWindow);
    if (parentWindow) memset(parentWindow, 0, sizeof(*parentWindow));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_put_ParentWindow(ICoreWebView2Controller4 *iface, HWND parentWindow)
{
    FIXME("iface %p, parentWindow %p stub!\n", iface, (void *)parentWindow);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_NotifyParentWindowPositionChanged(ICoreWebView2Controller4 *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_Close(ICoreWebView2Controller4 *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller_get_CoreWebView2(ICoreWebView2Controller4 *iface, ICoreWebView2 **coreWebView2)
{
    FIXME("iface %p, coreWebView2 %p stub!\n", iface, coreWebView2);
    if (coreWebView2) *coreWebView2 = stub_create(&stub_family_ICoreWebView2_29);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller2_get_DefaultBackgroundColor(ICoreWebView2Controller4 *iface, COREWEBVIEW2_COLOR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller2_put_DefaultBackgroundColor(ICoreWebView2Controller4 *iface, COREWEBVIEW2_COLOR value)
{
    FIXME("iface %p, value {...} stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller3_get_RasterizationScale(ICoreWebView2Controller4 *iface, double *scale)
{
    FIXME("iface %p, scale %p stub!\n", iface, scale);
    if (scale) memset(scale, 0, sizeof(*scale));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller3_put_RasterizationScale(ICoreWebView2Controller4 *iface, double scale)
{
    FIXME("iface %p, scale %f stub!\n", iface, scale);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller3_get_ShouldDetectMonitorScaleChanges(ICoreWebView2Controller4 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller3_put_ShouldDetectMonitorScaleChanges(ICoreWebView2Controller4 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller3_add_RasterizationScaleChanged(ICoreWebView2Controller4 *iface, ICoreWebView2RasterizationScaleChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller3_remove_RasterizationScaleChanged(ICoreWebView2Controller4 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller3_get_BoundsMode(ICoreWebView2Controller4 *iface, COREWEBVIEW2_BOUNDS_MODE *boundsMode)
{
    FIXME("iface %p, boundsMode %p stub!\n", iface, boundsMode);
    if (boundsMode) memset(boundsMode, 0, sizeof(*boundsMode));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller3_put_BoundsMode(ICoreWebView2Controller4 *iface, COREWEBVIEW2_BOUNDS_MODE boundsMode)
{
    FIXME("iface %p, boundsMode %d stub!\n", iface, (int)boundsMode);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller4_get_AllowExternalDrop(ICoreWebView2Controller4 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Controller4_put_AllowExternalDrop(ICoreWebView2Controller4 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static const ICoreWebView2Controller4Vtbl ICoreWebView2Controller4_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2Controller_get_IsVisible,
    ICoreWebView2Controller_put_IsVisible,
    ICoreWebView2Controller_get_Bounds,
    ICoreWebView2Controller_put_Bounds,
    ICoreWebView2Controller_get_ZoomFactor,
    ICoreWebView2Controller_put_ZoomFactor,
    ICoreWebView2Controller_add_ZoomFactorChanged,
    ICoreWebView2Controller_remove_ZoomFactorChanged,
    ICoreWebView2Controller_SetBoundsAndZoomFactor,
    ICoreWebView2Controller_MoveFocus,
    ICoreWebView2Controller_add_MoveFocusRequested,
    ICoreWebView2Controller_remove_MoveFocusRequested,
    ICoreWebView2Controller_add_GotFocus,
    ICoreWebView2Controller_remove_GotFocus,
    ICoreWebView2Controller_add_LostFocus,
    ICoreWebView2Controller_remove_LostFocus,
    ICoreWebView2Controller_add_AcceleratorKeyPressed,
    ICoreWebView2Controller_remove_AcceleratorKeyPressed,
    ICoreWebView2Controller_get_ParentWindow,
    ICoreWebView2Controller_put_ParentWindow,
    ICoreWebView2Controller_NotifyParentWindowPositionChanged,
    ICoreWebView2Controller_Close,
    ICoreWebView2Controller_get_CoreWebView2,
    ICoreWebView2Controller2_get_DefaultBackgroundColor,
    ICoreWebView2Controller2_put_DefaultBackgroundColor,
    ICoreWebView2Controller3_get_RasterizationScale,
    ICoreWebView2Controller3_put_RasterizationScale,
    ICoreWebView2Controller3_get_ShouldDetectMonitorScaleChanges,
    ICoreWebView2Controller3_put_ShouldDetectMonitorScaleChanges,
    ICoreWebView2Controller3_add_RasterizationScaleChanged,
    ICoreWebView2Controller3_remove_RasterizationScaleChanged,
    ICoreWebView2Controller3_get_BoundsMode,
    ICoreWebView2Controller3_put_BoundsMode,
    ICoreWebView2Controller4_get_AllowExternalDrop,
    ICoreWebView2Controller4_put_AllowExternalDrop,
};

const struct stub_family stub_family_ICoreWebView2Controller4 =
{
    "ICoreWebView2Controller4",
    &ICoreWebView2Controller4_stub_vtbl,
    ICoreWebView2Controller4_iids,
};

/* ===== ICoreWebView2ControllerOptions4 ===== */

static const IID *const ICoreWebView2ControllerOptions4_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ControllerOptions,
    &IID_ICoreWebView2ControllerOptions2,
    &IID_ICoreWebView2ControllerOptions3,
    &IID_ICoreWebView2ControllerOptions4,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ControllerOptions_get_ProfileName(ICoreWebView2ControllerOptions4 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ControllerOptions_put_ProfileName(ICoreWebView2ControllerOptions4 *iface, LPCWSTR value)
{
    FIXME("iface %p, value %s stub!\n", iface, debugstr_w(value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ControllerOptions_get_IsInPrivateModeEnabled(ICoreWebView2ControllerOptions4 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ControllerOptions_put_IsInPrivateModeEnabled(ICoreWebView2ControllerOptions4 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ControllerOptions2_get_ScriptLocale(ICoreWebView2ControllerOptions4 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ControllerOptions2_put_ScriptLocale(ICoreWebView2ControllerOptions4 *iface, LPCWSTR value)
{
    FIXME("iface %p, value %s stub!\n", iface, debugstr_w(value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ControllerOptions3_get_DefaultBackgroundColor(ICoreWebView2ControllerOptions4 *iface, COREWEBVIEW2_COLOR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ControllerOptions3_put_DefaultBackgroundColor(ICoreWebView2ControllerOptions4 *iface, COREWEBVIEW2_COLOR value)
{
    FIXME("iface %p, value {...} stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ControllerOptions4_get_AllowHostInputProcessing(ICoreWebView2ControllerOptions4 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ControllerOptions4_put_AllowHostInputProcessing(ICoreWebView2ControllerOptions4 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static const ICoreWebView2ControllerOptions4Vtbl ICoreWebView2ControllerOptions4_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ControllerOptions_get_ProfileName,
    ICoreWebView2ControllerOptions_put_ProfileName,
    ICoreWebView2ControllerOptions_get_IsInPrivateModeEnabled,
    ICoreWebView2ControllerOptions_put_IsInPrivateModeEnabled,
    ICoreWebView2ControllerOptions2_get_ScriptLocale,
    ICoreWebView2ControllerOptions2_put_ScriptLocale,
    ICoreWebView2ControllerOptions3_get_DefaultBackgroundColor,
    ICoreWebView2ControllerOptions3_put_DefaultBackgroundColor,
    ICoreWebView2ControllerOptions4_get_AllowHostInputProcessing,
    ICoreWebView2ControllerOptions4_put_AllowHostInputProcessing,
};

const struct stub_family stub_family_ICoreWebView2ControllerOptions4 =
{
    "ICoreWebView2ControllerOptions4",
    &ICoreWebView2ControllerOptions4_stub_vtbl,
    ICoreWebView2ControllerOptions4_iids,
};

/* ===== ICoreWebView2CrashReport ===== */

static const IID *const ICoreWebView2CrashReport_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2CrashReport,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2CrashReport_get_BucketId(ICoreWebView2CrashReport *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CrashReport_get_CrashReportId(ICoreWebView2CrashReport *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CrashReport_get_ExceptionCode(ICoreWebView2CrashReport *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CrashReport_get_FaultOffset(ICoreWebView2CrashReport *iface, UINT64 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CrashReport_get_FaultingModuleName(ICoreWebView2CrashReport *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CrashReport_get_FaultingModuleVersion(ICoreWebView2CrashReport *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2CrashReport_get_ReportTime(ICoreWebView2CrashReport *iface, UINT64 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2CrashReportVtbl ICoreWebView2CrashReport_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2CrashReport_get_BucketId,
    ICoreWebView2CrashReport_get_CrashReportId,
    ICoreWebView2CrashReport_get_ExceptionCode,
    ICoreWebView2CrashReport_get_FaultOffset,
    ICoreWebView2CrashReport_get_FaultingModuleName,
    ICoreWebView2CrashReport_get_FaultingModuleVersion,
    ICoreWebView2CrashReport_get_ReportTime,
};

const struct stub_family stub_family_ICoreWebView2CrashReport =
{
    "ICoreWebView2CrashReport",
    &ICoreWebView2CrashReport_stub_vtbl,
    ICoreWebView2CrashReport_iids,
};

/* ===== ICoreWebView2DOMContentLoadedEventArgs ===== */

static const IID *const ICoreWebView2DOMContentLoadedEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2DOMContentLoadedEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2DOMContentLoadedEventArgs_get_NavigationId(ICoreWebView2DOMContentLoadedEventArgs *iface, UINT64 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2DOMContentLoadedEventArgsVtbl ICoreWebView2DOMContentLoadedEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2DOMContentLoadedEventArgs_get_NavigationId,
};

const struct stub_family stub_family_ICoreWebView2DOMContentLoadedEventArgs =
{
    "ICoreWebView2DOMContentLoadedEventArgs",
    &ICoreWebView2DOMContentLoadedEventArgs_stub_vtbl,
    ICoreWebView2DOMContentLoadedEventArgs_iids,
};

/* ===== ICoreWebView2DedicatedWorker ===== */

static const IID *const ICoreWebView2DedicatedWorker_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2DedicatedWorker,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2DedicatedWorker_get_ScriptUri(ICoreWebView2DedicatedWorker *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DedicatedWorker_add_DedicatedWorkerCreated(ICoreWebView2DedicatedWorker *iface, ICoreWebView2DedicatedWorkerDedicatedWorkerCreatedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DedicatedWorker_remove_DedicatedWorkerCreated(ICoreWebView2DedicatedWorker *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DedicatedWorker_add_Destroying(ICoreWebView2DedicatedWorker *iface, ICoreWebView2DedicatedWorkerDestroyingEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DedicatedWorker_remove_Destroying(ICoreWebView2DedicatedWorker *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DedicatedWorker_add_WebMessageReceived(ICoreWebView2DedicatedWorker *iface, ICoreWebView2DedicatedWorkerWebMessageReceivedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DedicatedWorker_remove_WebMessageReceived(ICoreWebView2DedicatedWorker *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DedicatedWorker_PostWebMessageAsJson(ICoreWebView2DedicatedWorker *iface, LPCWSTR webMessageAsJson)
{
    FIXME("iface %p, webMessageAsJson %s stub!\n", iface, debugstr_w(webMessageAsJson));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DedicatedWorker_PostWebMessageAsString(ICoreWebView2DedicatedWorker *iface, LPCWSTR webMessageAsString)
{
    FIXME("iface %p, webMessageAsString %s stub!\n", iface, debugstr_w(webMessageAsString));
    return S_OK;
}

static const ICoreWebView2DedicatedWorkerVtbl ICoreWebView2DedicatedWorker_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2DedicatedWorker_get_ScriptUri,
    ICoreWebView2DedicatedWorker_add_DedicatedWorkerCreated,
    ICoreWebView2DedicatedWorker_remove_DedicatedWorkerCreated,
    ICoreWebView2DedicatedWorker_add_Destroying,
    ICoreWebView2DedicatedWorker_remove_Destroying,
    ICoreWebView2DedicatedWorker_add_WebMessageReceived,
    ICoreWebView2DedicatedWorker_remove_WebMessageReceived,
    ICoreWebView2DedicatedWorker_PostWebMessageAsJson,
    ICoreWebView2DedicatedWorker_PostWebMessageAsString,
};

const struct stub_family stub_family_ICoreWebView2DedicatedWorker =
{
    "ICoreWebView2DedicatedWorker",
    &ICoreWebView2DedicatedWorker_stub_vtbl,
    ICoreWebView2DedicatedWorker_iids,
};

/* ===== ICoreWebView2DedicatedWorkerCreatedEventArgs ===== */

static const IID *const ICoreWebView2DedicatedWorkerCreatedEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2DedicatedWorkerCreatedEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2DedicatedWorkerCreatedEventArgs_get_OriginalSourceFrameInfo(ICoreWebView2DedicatedWorkerCreatedEventArgs *iface, ICoreWebView2FrameInfo **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2FrameInfo2);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DedicatedWorkerCreatedEventArgs_get_Worker(ICoreWebView2DedicatedWorkerCreatedEventArgs *iface, ICoreWebView2DedicatedWorker **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2DedicatedWorker);
    return S_OK;
}

static const ICoreWebView2DedicatedWorkerCreatedEventArgsVtbl ICoreWebView2DedicatedWorkerCreatedEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2DedicatedWorkerCreatedEventArgs_get_OriginalSourceFrameInfo,
    ICoreWebView2DedicatedWorkerCreatedEventArgs_get_Worker,
};

const struct stub_family stub_family_ICoreWebView2DedicatedWorkerCreatedEventArgs =
{
    "ICoreWebView2DedicatedWorkerCreatedEventArgs",
    &ICoreWebView2DedicatedWorkerCreatedEventArgs_stub_vtbl,
    ICoreWebView2DedicatedWorkerCreatedEventArgs_iids,
};

/* ===== ICoreWebView2Deferral ===== */

static const IID *const ICoreWebView2Deferral_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2Deferral,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2Deferral_Complete(ICoreWebView2Deferral *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static const ICoreWebView2DeferralVtbl ICoreWebView2Deferral_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2Deferral_Complete,
};

const struct stub_family stub_family_ICoreWebView2Deferral =
{
    "ICoreWebView2Deferral",
    &ICoreWebView2Deferral_stub_vtbl,
    ICoreWebView2Deferral_iids,
};

/* ===== ICoreWebView2DevToolsProtocolEventReceivedEventArgs2 ===== */

static const IID *const ICoreWebView2DevToolsProtocolEventReceivedEventArgs2_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2DevToolsProtocolEventReceivedEventArgs,
    &IID_ICoreWebView2DevToolsProtocolEventReceivedEventArgs2,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2DevToolsProtocolEventReceivedEventArgs_get_ParameterObjectAsJson(ICoreWebView2DevToolsProtocolEventReceivedEventArgs2 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DevToolsProtocolEventReceivedEventArgs2_get_SessionId(ICoreWebView2DevToolsProtocolEventReceivedEventArgs2 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static const ICoreWebView2DevToolsProtocolEventReceivedEventArgs2Vtbl ICoreWebView2DevToolsProtocolEventReceivedEventArgs2_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2DevToolsProtocolEventReceivedEventArgs_get_ParameterObjectAsJson,
    ICoreWebView2DevToolsProtocolEventReceivedEventArgs2_get_SessionId,
};

const struct stub_family stub_family_ICoreWebView2DevToolsProtocolEventReceivedEventArgs2 =
{
    "ICoreWebView2DevToolsProtocolEventReceivedEventArgs2",
    &ICoreWebView2DevToolsProtocolEventReceivedEventArgs2_stub_vtbl,
    ICoreWebView2DevToolsProtocolEventReceivedEventArgs2_iids,
};

/* ===== ICoreWebView2DevToolsProtocolEventReceiver ===== */

static const IID *const ICoreWebView2DevToolsProtocolEventReceiver_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2DevToolsProtocolEventReceiver,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2DevToolsProtocolEventReceiver_add_DevToolsProtocolEventReceived(ICoreWebView2DevToolsProtocolEventReceiver *iface, ICoreWebView2DevToolsProtocolEventReceivedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DevToolsProtocolEventReceiver_remove_DevToolsProtocolEventReceived(ICoreWebView2DevToolsProtocolEventReceiver *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static const ICoreWebView2DevToolsProtocolEventReceiverVtbl ICoreWebView2DevToolsProtocolEventReceiver_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2DevToolsProtocolEventReceiver_add_DevToolsProtocolEventReceived,
    ICoreWebView2DevToolsProtocolEventReceiver_remove_DevToolsProtocolEventReceived,
};

const struct stub_family stub_family_ICoreWebView2DevToolsProtocolEventReceiver =
{
    "ICoreWebView2DevToolsProtocolEventReceiver",
    &ICoreWebView2DevToolsProtocolEventReceiver_stub_vtbl,
    ICoreWebView2DevToolsProtocolEventReceiver_iids,
};

/* ===== ICoreWebView2DragStartingEventArgs ===== */

static const IID *const ICoreWebView2DragStartingEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2DragStartingEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2DragStartingEventArgs_get_AllowedDropEffects(ICoreWebView2DragStartingEventArgs *iface, DWORD *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DragStartingEventArgs_get_Data(ICoreWebView2DragStartingEventArgs *iface, IDataObject **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = NULL;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DragStartingEventArgs_get_Handled(ICoreWebView2DragStartingEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DragStartingEventArgs_put_Handled(ICoreWebView2DragStartingEventArgs *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DragStartingEventArgs_get_Position(ICoreWebView2DragStartingEventArgs *iface, POINT *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2DragStartingEventArgs_GetDeferral(ICoreWebView2DragStartingEventArgs *iface, ICoreWebView2Deferral **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2Deferral);
    return S_OK;
}

static const ICoreWebView2DragStartingEventArgsVtbl ICoreWebView2DragStartingEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2DragStartingEventArgs_get_AllowedDropEffects,
    ICoreWebView2DragStartingEventArgs_get_Data,
    ICoreWebView2DragStartingEventArgs_get_Handled,
    ICoreWebView2DragStartingEventArgs_put_Handled,
    ICoreWebView2DragStartingEventArgs_get_Position,
    ICoreWebView2DragStartingEventArgs_GetDeferral,
};

const struct stub_family stub_family_ICoreWebView2DragStartingEventArgs =
{
    "ICoreWebView2DragStartingEventArgs",
    &ICoreWebView2DragStartingEventArgs_stub_vtbl,
    ICoreWebView2DragStartingEventArgs_iids,
};

/* ===== ICoreWebView2ProcessExtendedInfoCollection ===== */

static const IID *const ICoreWebView2ProcessExtendedInfoCollection_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ProcessExtendedInfoCollection,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ProcessExtendedInfoCollection_get_Count(ICoreWebView2ProcessExtendedInfoCollection *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ProcessExtendedInfoCollection_GetValueAtIndex(ICoreWebView2ProcessExtendedInfoCollection *iface, UINT32 index, ICoreWebView2ProcessExtendedInfo **value)
{
    FIXME("iface %p, index %d, value %p stub!\n", iface, (int)index, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ProcessExtendedInfo);
    return S_OK;
}

static const ICoreWebView2ProcessExtendedInfoCollectionVtbl ICoreWebView2ProcessExtendedInfoCollection_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ProcessExtendedInfoCollection_get_Count,
    ICoreWebView2ProcessExtendedInfoCollection_GetValueAtIndex,
};

const struct stub_family stub_family_ICoreWebView2ProcessExtendedInfoCollection =
{
    "ICoreWebView2ProcessExtendedInfoCollection",
    &ICoreWebView2ProcessExtendedInfoCollection_stub_vtbl,
    ICoreWebView2ProcessExtendedInfoCollection_iids,
};

/* ===== ICoreWebView2Environment15 ===== */

static const IID *const ICoreWebView2Environment15_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2Environment,
    &IID_ICoreWebView2Environment2,
    &IID_ICoreWebView2Environment3,
    &IID_ICoreWebView2Environment4,
    &IID_ICoreWebView2Environment5,
    &IID_ICoreWebView2Environment6,
    &IID_ICoreWebView2Environment7,
    &IID_ICoreWebView2Environment8,
    &IID_ICoreWebView2Environment9,
    &IID_ICoreWebView2Environment10,
    &IID_ICoreWebView2Environment11,
    &IID_ICoreWebView2Environment12,
    &IID_ICoreWebView2Environment13,
    &IID_ICoreWebView2Environment14,
    &IID_ICoreWebView2Environment15,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment_CreateCoreWebView2Controller(ICoreWebView2Environment15 *iface, HWND parentWindow, ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *handler)
{
    FIXME("iface %p, parentWindow %p, handler %p stub!\n", iface, (void *)parentWindow, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment_CreateWebResourceResponse(ICoreWebView2Environment15 *iface, IStream *content, int statusCode, LPCWSTR reasonPhrase, LPCWSTR headers, ICoreWebView2WebResourceResponse **response)
{
    FIXME("iface %p, content %p, statusCode %d, reasonPhrase %s, headers %s, response %p stub!\n", iface, (void *)content, (int)statusCode, debugstr_w(reasonPhrase), debugstr_w(headers), response);
    if (response) *response = stub_create(&stub_family_ICoreWebView2WebResourceResponse);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment_get_BrowserVersionString(ICoreWebView2Environment15 *iface, LPWSTR *versionInfo)
{
    FIXME("iface %p, versionInfo %p stub!\n", iface, versionInfo);
    if (versionInfo) *versionInfo = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment_add_NewBrowserVersionAvailable(ICoreWebView2Environment15 *iface, ICoreWebView2NewBrowserVersionAvailableEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment_remove_NewBrowserVersionAvailable(ICoreWebView2Environment15 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment2_CreateWebResourceRequest(ICoreWebView2Environment15 *iface, LPCWSTR uri, LPCWSTR Method, IStream *postData, LPCWSTR Headers, ICoreWebView2WebResourceRequest **value)
{
    FIXME("iface %p, uri %s, Method %s, postData %p, Headers %s, value %p stub!\n", iface, debugstr_w(uri), debugstr_w(Method), (void *)postData, debugstr_w(Headers), value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2WebResourceRequest);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment3_CreateCoreWebView2CompositionController(ICoreWebView2Environment15 *iface, HWND ParentWindow, ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler *handler)
{
    FIXME("iface %p, ParentWindow %p, handler %p stub!\n", iface, (void *)ParentWindow, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment3_CreateCoreWebView2PointerInfo(ICoreWebView2Environment15 *iface, ICoreWebView2PointerInfo **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2PointerInfo);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment4_GetAutomationProviderForWindow(ICoreWebView2Environment15 *iface, HWND hwnd, IUnknown **value)
{
    FIXME("iface %p, hwnd %p, value %p stub!\n", iface, (void *)hwnd, value);
    if (value) *value = NULL;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment5_add_BrowserProcessExited(ICoreWebView2Environment15 *iface, ICoreWebView2BrowserProcessExitedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment5_remove_BrowserProcessExited(ICoreWebView2Environment15 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment6_CreatePrintSettings(ICoreWebView2Environment15 *iface, ICoreWebView2PrintSettings **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2PrintSettings2);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment7_get_UserDataFolder(ICoreWebView2Environment15 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment8_add_ProcessInfosChanged(ICoreWebView2Environment15 *iface, ICoreWebView2ProcessInfosChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment8_remove_ProcessInfosChanged(ICoreWebView2Environment15 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment8_GetProcessInfos(ICoreWebView2Environment15 *iface, ICoreWebView2ProcessInfoCollection **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ProcessInfoCollection);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment9_CreateContextMenuItem(ICoreWebView2Environment15 *iface, LPCWSTR Label, IStream *iconStream, COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND Kind, ICoreWebView2ContextMenuItem **value)
{
    FIXME("iface %p, Label %s, iconStream %p, Kind %d, value %p stub!\n", iface, debugstr_w(Label), (void *)iconStream, (int)Kind, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ContextMenuItem);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment10_CreateCoreWebView2ControllerOptions(ICoreWebView2Environment15 *iface, ICoreWebView2ControllerOptions **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ControllerOptions4);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment10_CreateCoreWebView2ControllerWithOptions(ICoreWebView2Environment15 *iface, HWND ParentWindow, ICoreWebView2ControllerOptions *options, ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *handler)
{
    FIXME("iface %p, ParentWindow %p, options %p, handler %p stub!\n", iface, (void *)ParentWindow, (void *)options, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment10_CreateCoreWebView2CompositionControllerWithOptions(ICoreWebView2Environment15 *iface, HWND ParentWindow, ICoreWebView2ControllerOptions *options, ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler *handler)
{
    FIXME("iface %p, ParentWindow %p, options %p, handler %p stub!\n", iface, (void *)ParentWindow, (void *)options, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment11_get_FailureReportFolderPath(ICoreWebView2Environment15 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment12_CreateSharedBuffer(ICoreWebView2Environment15 *iface, UINT64 Size, ICoreWebView2SharedBuffer **value)
{
    FIXME("iface %p, Size %s, value %p stub!\n", iface, wine_dbgstr_longlong(Size), value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2SharedBuffer);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment13_GetProcessExtendedInfos(ICoreWebView2Environment15 *iface, ICoreWebView2GetProcessExtendedInfosCompletedHandler *handler)
{
    FIXME("iface %p, handler %p stub!\n", iface, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2GetProcessExtendedInfosCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment14_CreateWebFileSystemFileHandle(ICoreWebView2Environment15 *iface, LPCWSTR path, COREWEBVIEW2_FILE_SYSTEM_HANDLE_PERMISSION permission, ICoreWebView2FileSystemHandle **value)
{
    FIXME("iface %p, path %s, permission %d, value %p stub!\n", iface, debugstr_w(path), (int)permission, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2FileSystemHandle);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment14_CreateWebFileSystemDirectoryHandle(ICoreWebView2Environment15 *iface, LPCWSTR path, COREWEBVIEW2_FILE_SYSTEM_HANDLE_PERMISSION permission, ICoreWebView2FileSystemHandle **value)
{
    FIXME("iface %p, path %s, permission %d, value %p stub!\n", iface, debugstr_w(path), (int)permission, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2FileSystemHandle);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment14_CreateObjectCollection(ICoreWebView2Environment15 *iface, UINT32 length, IUnknown **items, ICoreWebView2ObjectCollection **objectCollection)
{
    FIXME("iface %p, length %d, items %p, objectCollection %p stub!\n", iface, (int)length, (void *)items, objectCollection);
    if (objectCollection) *objectCollection = stub_create(&stub_family_ICoreWebView2ObjectCollection);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Environment15_CreateFindOptions(ICoreWebView2Environment15 *iface, ICoreWebView2FindOptions **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2FindOptions);
    return S_OK;
}

static const ICoreWebView2Environment15Vtbl ICoreWebView2Environment15_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2Environment_CreateCoreWebView2Controller,
    ICoreWebView2Environment_CreateWebResourceResponse,
    ICoreWebView2Environment_get_BrowserVersionString,
    ICoreWebView2Environment_add_NewBrowserVersionAvailable,
    ICoreWebView2Environment_remove_NewBrowserVersionAvailable,
    ICoreWebView2Environment2_CreateWebResourceRequest,
    ICoreWebView2Environment3_CreateCoreWebView2CompositionController,
    ICoreWebView2Environment3_CreateCoreWebView2PointerInfo,
    ICoreWebView2Environment4_GetAutomationProviderForWindow,
    ICoreWebView2Environment5_add_BrowserProcessExited,
    ICoreWebView2Environment5_remove_BrowserProcessExited,
    ICoreWebView2Environment6_CreatePrintSettings,
    ICoreWebView2Environment7_get_UserDataFolder,
    ICoreWebView2Environment8_add_ProcessInfosChanged,
    ICoreWebView2Environment8_remove_ProcessInfosChanged,
    ICoreWebView2Environment8_GetProcessInfos,
    ICoreWebView2Environment9_CreateContextMenuItem,
    ICoreWebView2Environment10_CreateCoreWebView2ControllerOptions,
    ICoreWebView2Environment10_CreateCoreWebView2ControllerWithOptions,
    ICoreWebView2Environment10_CreateCoreWebView2CompositionControllerWithOptions,
    ICoreWebView2Environment11_get_FailureReportFolderPath,
    ICoreWebView2Environment12_CreateSharedBuffer,
    ICoreWebView2Environment13_GetProcessExtendedInfos,
    ICoreWebView2Environment14_CreateWebFileSystemFileHandle,
    ICoreWebView2Environment14_CreateWebFileSystemDirectoryHandle,
    ICoreWebView2Environment14_CreateObjectCollection,
    ICoreWebView2Environment15_CreateFindOptions,
};

const struct stub_family stub_family_ICoreWebView2Environment15 =
{
    "ICoreWebView2Environment15",
    &ICoreWebView2Environment15_stub_vtbl,
    ICoreWebView2Environment15_iids,
};

/* ===== ICoreWebView2ProcessInfoCollection ===== */

static const IID *const ICoreWebView2ProcessInfoCollection_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ProcessInfoCollection,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ProcessInfoCollection_get_Count(ICoreWebView2ProcessInfoCollection *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ProcessInfoCollection_GetValueAtIndex(ICoreWebView2ProcessInfoCollection *iface, UINT32 index, ICoreWebView2ProcessInfo **value)
{
    FIXME("iface %p, index %d, value %p stub!\n", iface, (int)index, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ProcessInfo);
    return S_OK;
}

static const ICoreWebView2ProcessInfoCollectionVtbl ICoreWebView2ProcessInfoCollection_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ProcessInfoCollection_get_Count,
    ICoreWebView2ProcessInfoCollection_GetValueAtIndex,
};

const struct stub_family stub_family_ICoreWebView2ProcessInfoCollection =
{
    "ICoreWebView2ProcessInfoCollection",
    &ICoreWebView2ProcessInfoCollection_stub_vtbl,
    ICoreWebView2ProcessInfoCollection_iids,
};

/* ===== ICoreWebView2File ===== */

static const IID *const ICoreWebView2File_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2File,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2File_get_Path(ICoreWebView2File *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static const ICoreWebView2FileVtbl ICoreWebView2File_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2File_get_Path,
};

const struct stub_family stub_family_ICoreWebView2File =
{
    "ICoreWebView2File",
    &ICoreWebView2File_stub_vtbl,
    ICoreWebView2File_iids,
};

/* ===== ICoreWebView2FileSystemHandle ===== */

static const IID *const ICoreWebView2FileSystemHandle_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2FileSystemHandle,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2FileSystemHandle_get_Kind(ICoreWebView2FileSystemHandle *iface, COREWEBVIEW2_FILE_SYSTEM_HANDLE_KIND *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FileSystemHandle_get_Path(ICoreWebView2FileSystemHandle *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FileSystemHandle_get_Permission(ICoreWebView2FileSystemHandle *iface, COREWEBVIEW2_FILE_SYSTEM_HANDLE_PERMISSION *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2FileSystemHandleVtbl ICoreWebView2FileSystemHandle_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2FileSystemHandle_get_Kind,
    ICoreWebView2FileSystemHandle_get_Path,
    ICoreWebView2FileSystemHandle_get_Permission,
};

const struct stub_family stub_family_ICoreWebView2FileSystemHandle =
{
    "ICoreWebView2FileSystemHandle",
    &ICoreWebView2FileSystemHandle_stub_vtbl,
    ICoreWebView2FileSystemHandle_iids,
};

/* ===== ICoreWebView2Find ===== */

static const IID *const ICoreWebView2Find_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2Find,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2Find_get_ActiveMatchIndex(ICoreWebView2Find *iface, INT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Find_get_MatchCount(ICoreWebView2Find *iface, INT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Find_add_ActiveMatchIndexChanged(ICoreWebView2Find *iface, ICoreWebView2FindActiveMatchIndexChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Find_remove_ActiveMatchIndexChanged(ICoreWebView2Find *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Find_add_MatchCountChanged(ICoreWebView2Find *iface, ICoreWebView2FindMatchCountChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Find_remove_MatchCountChanged(ICoreWebView2Find *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Find_Start(ICoreWebView2Find *iface, ICoreWebView2FindOptions *options, ICoreWebView2FindStartCompletedHandler *handler)
{
    FIXME("iface %p, options %p, handler %p stub!\n", iface, (void *)options, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2FindStartCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Find_FindNext(ICoreWebView2Find *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Find_FindPrevious(ICoreWebView2Find *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Find_Stop(ICoreWebView2Find *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static const ICoreWebView2FindVtbl ICoreWebView2Find_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2Find_get_ActiveMatchIndex,
    ICoreWebView2Find_get_MatchCount,
    ICoreWebView2Find_add_ActiveMatchIndexChanged,
    ICoreWebView2Find_remove_ActiveMatchIndexChanged,
    ICoreWebView2Find_add_MatchCountChanged,
    ICoreWebView2Find_remove_MatchCountChanged,
    ICoreWebView2Find_Start,
    ICoreWebView2Find_FindNext,
    ICoreWebView2Find_FindPrevious,
    ICoreWebView2Find_Stop,
};

const struct stub_family stub_family_ICoreWebView2Find =
{
    "ICoreWebView2Find",
    &ICoreWebView2Find_stub_vtbl,
    ICoreWebView2Find_iids,
};

/* ===== ICoreWebView2FindOptions ===== */

static const IID *const ICoreWebView2FindOptions_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2FindOptions,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2FindOptions_get_FindTerm(ICoreWebView2FindOptions *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FindOptions_put_FindTerm(ICoreWebView2FindOptions *iface, LPCWSTR value)
{
    FIXME("iface %p, value %s stub!\n", iface, debugstr_w(value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FindOptions_get_IsCaseSensitive(ICoreWebView2FindOptions *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FindOptions_put_IsCaseSensitive(ICoreWebView2FindOptions *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FindOptions_get_ShouldHighlightAllMatches(ICoreWebView2FindOptions *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FindOptions_put_ShouldHighlightAllMatches(ICoreWebView2FindOptions *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FindOptions_get_ShouldMatchWord(ICoreWebView2FindOptions *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FindOptions_put_ShouldMatchWord(ICoreWebView2FindOptions *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FindOptions_get_SuppressDefaultFindDialog(ICoreWebView2FindOptions *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FindOptions_put_SuppressDefaultFindDialog(ICoreWebView2FindOptions *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static const ICoreWebView2FindOptionsVtbl ICoreWebView2FindOptions_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2FindOptions_get_FindTerm,
    ICoreWebView2FindOptions_put_FindTerm,
    ICoreWebView2FindOptions_get_IsCaseSensitive,
    ICoreWebView2FindOptions_put_IsCaseSensitive,
    ICoreWebView2FindOptions_get_ShouldHighlightAllMatches,
    ICoreWebView2FindOptions_put_ShouldHighlightAllMatches,
    ICoreWebView2FindOptions_get_ShouldMatchWord,
    ICoreWebView2FindOptions_put_ShouldMatchWord,
    ICoreWebView2FindOptions_get_SuppressDefaultFindDialog,
    ICoreWebView2FindOptions_put_SuppressDefaultFindDialog,
};

const struct stub_family stub_family_ICoreWebView2FindOptions =
{
    "ICoreWebView2FindOptions",
    &ICoreWebView2FindOptions_stub_vtbl,
    ICoreWebView2FindOptions_iids,
};

/* ===== ICoreWebView2Frame8 ===== */

static const IID *const ICoreWebView2Frame8_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2Frame,
    &IID_ICoreWebView2Frame2,
    &IID_ICoreWebView2Frame3,
    &IID_ICoreWebView2Frame4,
    &IID_ICoreWebView2Frame5,
    &IID_ICoreWebView2Frame6,
    &IID_ICoreWebView2Frame7,
    &IID_ICoreWebView2Frame8,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame_get_Name(ICoreWebView2Frame8 *iface, LPWSTR *name)
{
    FIXME("iface %p, name %p stub!\n", iface, name);
    if (name) *name = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame_add_NameChanged(ICoreWebView2Frame8 *iface, ICoreWebView2FrameNameChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame_remove_NameChanged(ICoreWebView2Frame8 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame_AddHostObjectToScriptWithOrigins(ICoreWebView2Frame8 *iface, LPCWSTR name, VARIANT *object, UINT32 originsCount, LPCWSTR *origins)
{
    FIXME("iface %p, name %s, object %p, originsCount %d, origins %p stub!\n", iface, debugstr_w(name), (void *)object, (int)originsCount, (void *)origins);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame_RemoveHostObjectFromScript(ICoreWebView2Frame8 *iface, LPCWSTR name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_w(name));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame_add_Destroyed(ICoreWebView2Frame8 *iface, ICoreWebView2FrameDestroyedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame_remove_Destroyed(ICoreWebView2Frame8 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame_IsDestroyed(ICoreWebView2Frame8 *iface, BOOL *destroyed)
{
    FIXME("iface %p, destroyed %p stub!\n", iface, destroyed);
    if (destroyed) memset(destroyed, 0, sizeof(*destroyed));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame2_add_NavigationStarting(ICoreWebView2Frame8 *iface, ICoreWebView2FrameNavigationStartingEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame2_remove_NavigationStarting(ICoreWebView2Frame8 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame2_add_ContentLoading(ICoreWebView2Frame8 *iface, ICoreWebView2FrameContentLoadingEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame2_remove_ContentLoading(ICoreWebView2Frame8 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame2_add_NavigationCompleted(ICoreWebView2Frame8 *iface, ICoreWebView2FrameNavigationCompletedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame2_remove_NavigationCompleted(ICoreWebView2Frame8 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame2_add_DOMContentLoaded(ICoreWebView2Frame8 *iface, ICoreWebView2FrameDOMContentLoadedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame2_remove_DOMContentLoaded(ICoreWebView2Frame8 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame2_ExecuteScript(ICoreWebView2Frame8 *iface, LPCWSTR javaScript, ICoreWebView2ExecuteScriptCompletedHandler *handler)
{
    FIXME("iface %p, javaScript %s, handler %p stub!\n", iface, debugstr_w(javaScript), (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2ExecuteScriptCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame2_PostWebMessageAsJson(ICoreWebView2Frame8 *iface, LPCWSTR webMessageAsJson)
{
    FIXME("iface %p, webMessageAsJson %s stub!\n", iface, debugstr_w(webMessageAsJson));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame2_PostWebMessageAsString(ICoreWebView2Frame8 *iface, LPCWSTR webMessageAsString)
{
    FIXME("iface %p, webMessageAsString %s stub!\n", iface, debugstr_w(webMessageAsString));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame2_add_WebMessageReceived(ICoreWebView2Frame8 *iface, ICoreWebView2FrameWebMessageReceivedEventHandler *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub!\n", iface, (void *)handler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame2_remove_WebMessageReceived(ICoreWebView2Frame8 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame3_add_PermissionRequested(ICoreWebView2Frame8 *iface, ICoreWebView2FramePermissionRequestedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame3_remove_PermissionRequested(ICoreWebView2Frame8 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame4_PostSharedBufferToScript(ICoreWebView2Frame8 *iface, ICoreWebView2SharedBuffer *sharedBuffer, COREWEBVIEW2_SHARED_BUFFER_ACCESS access, LPCWSTR additionalDataAsJson)
{
    FIXME("iface %p, sharedBuffer %p, access %d, additionalDataAsJson %s stub!\n", iface, (void *)sharedBuffer, (int)access, debugstr_w(additionalDataAsJson));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame5_get_FrameId(ICoreWebView2Frame8 *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame6_add_ScreenCaptureStarting(ICoreWebView2Frame8 *iface, ICoreWebView2FrameScreenCaptureStartingEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame6_remove_ScreenCaptureStarting(ICoreWebView2Frame8 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame7_add_FrameCreated(ICoreWebView2Frame8 *iface, ICoreWebView2FrameChildFrameCreatedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame7_remove_FrameCreated(ICoreWebView2Frame8 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame8_add_DedicatedWorkerCreated(ICoreWebView2Frame8 *iface, ICoreWebView2FrameDedicatedWorkerCreatedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Frame8_remove_DedicatedWorkerCreated(ICoreWebView2Frame8 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static const ICoreWebView2Frame8Vtbl ICoreWebView2Frame8_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2Frame_get_Name,
    ICoreWebView2Frame_add_NameChanged,
    ICoreWebView2Frame_remove_NameChanged,
    ICoreWebView2Frame_AddHostObjectToScriptWithOrigins,
    ICoreWebView2Frame_RemoveHostObjectFromScript,
    ICoreWebView2Frame_add_Destroyed,
    ICoreWebView2Frame_remove_Destroyed,
    ICoreWebView2Frame_IsDestroyed,
    ICoreWebView2Frame2_add_NavigationStarting,
    ICoreWebView2Frame2_remove_NavigationStarting,
    ICoreWebView2Frame2_add_ContentLoading,
    ICoreWebView2Frame2_remove_ContentLoading,
    ICoreWebView2Frame2_add_NavigationCompleted,
    ICoreWebView2Frame2_remove_NavigationCompleted,
    ICoreWebView2Frame2_add_DOMContentLoaded,
    ICoreWebView2Frame2_remove_DOMContentLoaded,
    ICoreWebView2Frame2_ExecuteScript,
    ICoreWebView2Frame2_PostWebMessageAsJson,
    ICoreWebView2Frame2_PostWebMessageAsString,
    ICoreWebView2Frame2_add_WebMessageReceived,
    ICoreWebView2Frame2_remove_WebMessageReceived,
    ICoreWebView2Frame3_add_PermissionRequested,
    ICoreWebView2Frame3_remove_PermissionRequested,
    ICoreWebView2Frame4_PostSharedBufferToScript,
    ICoreWebView2Frame5_get_FrameId,
    ICoreWebView2Frame6_add_ScreenCaptureStarting,
    ICoreWebView2Frame6_remove_ScreenCaptureStarting,
    ICoreWebView2Frame7_add_FrameCreated,
    ICoreWebView2Frame7_remove_FrameCreated,
    ICoreWebView2Frame8_add_DedicatedWorkerCreated,
    ICoreWebView2Frame8_remove_DedicatedWorkerCreated,
};

const struct stub_family stub_family_ICoreWebView2Frame8 =
{
    "ICoreWebView2Frame8",
    &ICoreWebView2Frame8_stub_vtbl,
    ICoreWebView2Frame8_iids,
};

/* ===== ICoreWebView2FrameCreatedEventArgs ===== */

static const IID *const ICoreWebView2FrameCreatedEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2FrameCreatedEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2FrameCreatedEventArgs_get_Frame(ICoreWebView2FrameCreatedEventArgs *iface, ICoreWebView2Frame **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2Frame8);
    return S_OK;
}

static const ICoreWebView2FrameCreatedEventArgsVtbl ICoreWebView2FrameCreatedEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2FrameCreatedEventArgs_get_Frame,
};

const struct stub_family stub_family_ICoreWebView2FrameCreatedEventArgs =
{
    "ICoreWebView2FrameCreatedEventArgs",
    &ICoreWebView2FrameCreatedEventArgs_stub_vtbl,
    ICoreWebView2FrameCreatedEventArgs_iids,
};

/* ===== ICoreWebView2FrameInfoCollection ===== */

static const IID *const ICoreWebView2FrameInfoCollection_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2FrameInfoCollection,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2FrameInfoCollection_GetIterator(ICoreWebView2FrameInfoCollection *iface, ICoreWebView2FrameInfoCollectionIterator **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2FrameInfoCollectionIterator);
    return S_OK;
}

static const ICoreWebView2FrameInfoCollectionVtbl ICoreWebView2FrameInfoCollection_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2FrameInfoCollection_GetIterator,
};

const struct stub_family stub_family_ICoreWebView2FrameInfoCollection =
{
    "ICoreWebView2FrameInfoCollection",
    &ICoreWebView2FrameInfoCollection_stub_vtbl,
    ICoreWebView2FrameInfoCollection_iids,
};

/* ===== ICoreWebView2FrameInfoCollectionIterator ===== */

static const IID *const ICoreWebView2FrameInfoCollectionIterator_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2FrameInfoCollectionIterator,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2FrameInfoCollectionIterator_get_HasCurrent(ICoreWebView2FrameInfoCollectionIterator *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FrameInfoCollectionIterator_GetCurrent(ICoreWebView2FrameInfoCollectionIterator *iface, ICoreWebView2FrameInfo **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2FrameInfo2);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2FrameInfoCollectionIterator_MoveNext(ICoreWebView2FrameInfoCollectionIterator *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2FrameInfoCollectionIteratorVtbl ICoreWebView2FrameInfoCollectionIterator_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2FrameInfoCollectionIterator_get_HasCurrent,
    ICoreWebView2FrameInfoCollectionIterator_GetCurrent,
    ICoreWebView2FrameInfoCollectionIterator_MoveNext,
};

const struct stub_family stub_family_ICoreWebView2FrameInfoCollectionIterator =
{
    "ICoreWebView2FrameInfoCollectionIterator",
    &ICoreWebView2FrameInfoCollectionIterator_stub_vtbl,
    ICoreWebView2FrameInfoCollectionIterator_iids,
};

/* ===== ICoreWebView2HttpRequestHeaders ===== */

static const IID *const ICoreWebView2HttpRequestHeaders_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2HttpRequestHeaders,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2HttpRequestHeaders_GetHeader(ICoreWebView2HttpRequestHeaders *iface, LPCWSTR name, LPWSTR *value)
{
    FIXME("iface %p, name %s, value %p stub!\n", iface, debugstr_w(name), value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2HttpRequestHeaders_GetHeaders(ICoreWebView2HttpRequestHeaders *iface, LPCWSTR name, ICoreWebView2HttpHeadersCollectionIterator **value)
{
    FIXME("iface %p, name %s, value %p stub!\n", iface, debugstr_w(name), value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2HttpHeadersCollectionIterator);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2HttpRequestHeaders_Contains(ICoreWebView2HttpRequestHeaders *iface, LPCWSTR name, BOOL *value)
{
    FIXME("iface %p, name %s, value %p stub!\n", iface, debugstr_w(name), value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2HttpRequestHeaders_SetHeader(ICoreWebView2HttpRequestHeaders *iface, LPCWSTR name, LPCWSTR value)
{
    FIXME("iface %p, name %s, value %s stub!\n", iface, debugstr_w(name), debugstr_w(value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2HttpRequestHeaders_RemoveHeader(ICoreWebView2HttpRequestHeaders *iface, LPCWSTR name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_w(name));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2HttpRequestHeaders_GetIterator(ICoreWebView2HttpRequestHeaders *iface, ICoreWebView2HttpHeadersCollectionIterator **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2HttpHeadersCollectionIterator);
    return S_OK;
}

static const ICoreWebView2HttpRequestHeadersVtbl ICoreWebView2HttpRequestHeaders_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2HttpRequestHeaders_GetHeader,
    ICoreWebView2HttpRequestHeaders_GetHeaders,
    ICoreWebView2HttpRequestHeaders_Contains,
    ICoreWebView2HttpRequestHeaders_SetHeader,
    ICoreWebView2HttpRequestHeaders_RemoveHeader,
    ICoreWebView2HttpRequestHeaders_GetIterator,
};

const struct stub_family stub_family_ICoreWebView2HttpRequestHeaders =
{
    "ICoreWebView2HttpRequestHeaders",
    &ICoreWebView2HttpRequestHeaders_stub_vtbl,
    ICoreWebView2HttpRequestHeaders_iids,
};

/* ===== ICoreWebView2HttpResponseHeaders ===== */

static const IID *const ICoreWebView2HttpResponseHeaders_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2HttpResponseHeaders,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2HttpResponseHeaders_AppendHeader(ICoreWebView2HttpResponseHeaders *iface, LPCWSTR name, LPCWSTR value)
{
    FIXME("iface %p, name %s, value %s stub!\n", iface, debugstr_w(name), debugstr_w(value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2HttpResponseHeaders_Contains(ICoreWebView2HttpResponseHeaders *iface, LPCWSTR name, BOOL *value)
{
    FIXME("iface %p, name %s, value %p stub!\n", iface, debugstr_w(name), value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2HttpResponseHeaders_GetHeader(ICoreWebView2HttpResponseHeaders *iface, LPCWSTR name, LPWSTR *value)
{
    FIXME("iface %p, name %s, value %p stub!\n", iface, debugstr_w(name), value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2HttpResponseHeaders_GetHeaders(ICoreWebView2HttpResponseHeaders *iface, LPCWSTR name, ICoreWebView2HttpHeadersCollectionIterator **value)
{
    FIXME("iface %p, name %s, value %p stub!\n", iface, debugstr_w(name), value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2HttpHeadersCollectionIterator);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2HttpResponseHeaders_GetIterator(ICoreWebView2HttpResponseHeaders *iface, ICoreWebView2HttpHeadersCollectionIterator **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2HttpHeadersCollectionIterator);
    return S_OK;
}

static const ICoreWebView2HttpResponseHeadersVtbl ICoreWebView2HttpResponseHeaders_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2HttpResponseHeaders_AppendHeader,
    ICoreWebView2HttpResponseHeaders_Contains,
    ICoreWebView2HttpResponseHeaders_GetHeader,
    ICoreWebView2HttpResponseHeaders_GetHeaders,
    ICoreWebView2HttpResponseHeaders_GetIterator,
};

const struct stub_family stub_family_ICoreWebView2HttpResponseHeaders =
{
    "ICoreWebView2HttpResponseHeaders",
    &ICoreWebView2HttpResponseHeaders_stub_vtbl,
    ICoreWebView2HttpResponseHeaders_iids,
};

/* ===== ICoreWebView2NavigationCompletedEventArgs2 ===== */

static const IID *const ICoreWebView2NavigationCompletedEventArgs2_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2NavigationCompletedEventArgs,
    &IID_ICoreWebView2NavigationCompletedEventArgs2,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2NavigationCompletedEventArgs_get_IsSuccess(ICoreWebView2NavigationCompletedEventArgs2 *iface, BOOL *isSuccess)
{
    FIXME("iface %p, isSuccess %p stub!\n", iface, isSuccess);
    if (isSuccess) memset(isSuccess, 0, sizeof(*isSuccess));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NavigationCompletedEventArgs_get_WebErrorStatus(ICoreWebView2NavigationCompletedEventArgs2 *iface, COREWEBVIEW2_WEB_ERROR_STATUS *webErrorStatus)
{
    FIXME("iface %p, webErrorStatus %p stub!\n", iface, webErrorStatus);
    if (webErrorStatus) memset(webErrorStatus, 0, sizeof(*webErrorStatus));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NavigationCompletedEventArgs_get_NavigationId(ICoreWebView2NavigationCompletedEventArgs2 *iface, UINT64 *navigationId)
{
    FIXME("iface %p, navigationId %p stub!\n", iface, navigationId);
    if (navigationId) memset(navigationId, 0, sizeof(*navigationId));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NavigationCompletedEventArgs2_get_HttpStatusCode(ICoreWebView2NavigationCompletedEventArgs2 *iface, int *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2NavigationCompletedEventArgs2Vtbl ICoreWebView2NavigationCompletedEventArgs2_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2NavigationCompletedEventArgs_get_IsSuccess,
    ICoreWebView2NavigationCompletedEventArgs_get_WebErrorStatus,
    ICoreWebView2NavigationCompletedEventArgs_get_NavigationId,
    ICoreWebView2NavigationCompletedEventArgs2_get_HttpStatusCode,
};

const struct stub_family stub_family_ICoreWebView2NavigationCompletedEventArgs2 =
{
    "ICoreWebView2NavigationCompletedEventArgs2",
    &ICoreWebView2NavigationCompletedEventArgs2_stub_vtbl,
    ICoreWebView2NavigationCompletedEventArgs2_iids,
};

/* ===== ICoreWebView2NavigationStartingEventArgs3 ===== */

static const IID *const ICoreWebView2NavigationStartingEventArgs3_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2NavigationStartingEventArgs,
    &IID_ICoreWebView2NavigationStartingEventArgs2,
    &IID_ICoreWebView2NavigationStartingEventArgs3,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2NavigationStartingEventArgs_get_Uri(ICoreWebView2NavigationStartingEventArgs3 *iface, LPWSTR *uri)
{
    FIXME("iface %p, uri %p stub!\n", iface, uri);
    if (uri) *uri = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NavigationStartingEventArgs_get_IsUserInitiated(ICoreWebView2NavigationStartingEventArgs3 *iface, BOOL *isUserInitiated)
{
    FIXME("iface %p, isUserInitiated %p stub!\n", iface, isUserInitiated);
    if (isUserInitiated) memset(isUserInitiated, 0, sizeof(*isUserInitiated));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NavigationStartingEventArgs_get_IsRedirected(ICoreWebView2NavigationStartingEventArgs3 *iface, BOOL *isRedirected)
{
    FIXME("iface %p, isRedirected %p stub!\n", iface, isRedirected);
    if (isRedirected) memset(isRedirected, 0, sizeof(*isRedirected));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NavigationStartingEventArgs_get_RequestHeaders(ICoreWebView2NavigationStartingEventArgs3 *iface, ICoreWebView2HttpRequestHeaders **requestHeaders)
{
    FIXME("iface %p, requestHeaders %p stub!\n", iface, requestHeaders);
    if (requestHeaders) *requestHeaders = stub_create(&stub_family_ICoreWebView2HttpRequestHeaders);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NavigationStartingEventArgs_get_Cancel(ICoreWebView2NavigationStartingEventArgs3 *iface, BOOL *cancel)
{
    FIXME("iface %p, cancel %p stub!\n", iface, cancel);
    if (cancel) memset(cancel, 0, sizeof(*cancel));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NavigationStartingEventArgs_put_Cancel(ICoreWebView2NavigationStartingEventArgs3 *iface, BOOL cancel)
{
    FIXME("iface %p, cancel %d stub!\n", iface, (int)cancel);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NavigationStartingEventArgs_get_NavigationId(ICoreWebView2NavigationStartingEventArgs3 *iface, UINT64 *navigationId)
{
    FIXME("iface %p, navigationId %p stub!\n", iface, navigationId);
    if (navigationId) memset(navigationId, 0, sizeof(*navigationId));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NavigationStartingEventArgs2_get_AdditionalAllowedFrameAncestors(ICoreWebView2NavigationStartingEventArgs3 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NavigationStartingEventArgs2_put_AdditionalAllowedFrameAncestors(ICoreWebView2NavigationStartingEventArgs3 *iface, LPCWSTR value)
{
    FIXME("iface %p, value %s stub!\n", iface, debugstr_w(value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NavigationStartingEventArgs3_get_NavigationKind(ICoreWebView2NavigationStartingEventArgs3 *iface, COREWEBVIEW2_NAVIGATION_KIND *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2NavigationStartingEventArgs3Vtbl ICoreWebView2NavigationStartingEventArgs3_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2NavigationStartingEventArgs_get_Uri,
    ICoreWebView2NavigationStartingEventArgs_get_IsUserInitiated,
    ICoreWebView2NavigationStartingEventArgs_get_IsRedirected,
    ICoreWebView2NavigationStartingEventArgs_get_RequestHeaders,
    ICoreWebView2NavigationStartingEventArgs_get_Cancel,
    ICoreWebView2NavigationStartingEventArgs_put_Cancel,
    ICoreWebView2NavigationStartingEventArgs_get_NavigationId,
    ICoreWebView2NavigationStartingEventArgs2_get_AdditionalAllowedFrameAncestors,
    ICoreWebView2NavigationStartingEventArgs2_put_AdditionalAllowedFrameAncestors,
    ICoreWebView2NavigationStartingEventArgs3_get_NavigationKind,
};

const struct stub_family stub_family_ICoreWebView2NavigationStartingEventArgs3 =
{
    "ICoreWebView2NavigationStartingEventArgs3",
    &ICoreWebView2NavigationStartingEventArgs3_stub_vtbl,
    ICoreWebView2NavigationStartingEventArgs3_iids,
};

/* ===== ICoreWebView2NewWindowRequestedEventArgs3 ===== */

static const IID *const ICoreWebView2NewWindowRequestedEventArgs3_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2NewWindowRequestedEventArgs,
    &IID_ICoreWebView2NewWindowRequestedEventArgs2,
    &IID_ICoreWebView2NewWindowRequestedEventArgs3,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2NewWindowRequestedEventArgs_get_Uri(ICoreWebView2NewWindowRequestedEventArgs3 *iface, LPWSTR *uri)
{
    FIXME("iface %p, uri %p stub!\n", iface, uri);
    if (uri) *uri = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NewWindowRequestedEventArgs_put_NewWindow(ICoreWebView2NewWindowRequestedEventArgs3 *iface, ICoreWebView2 *newWindow)
{
    FIXME("iface %p, newWindow %p stub!\n", iface, (void *)newWindow);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NewWindowRequestedEventArgs_get_NewWindow(ICoreWebView2NewWindowRequestedEventArgs3 *iface, ICoreWebView2 **newWindow)
{
    FIXME("iface %p, newWindow %p stub!\n", iface, newWindow);
    if (newWindow) *newWindow = stub_create(&stub_family_ICoreWebView2_29);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NewWindowRequestedEventArgs_put_Handled(ICoreWebView2NewWindowRequestedEventArgs3 *iface, BOOL handled)
{
    FIXME("iface %p, handled %d stub!\n", iface, (int)handled);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NewWindowRequestedEventArgs_get_Handled(ICoreWebView2NewWindowRequestedEventArgs3 *iface, BOOL *handled)
{
    FIXME("iface %p, handled %p stub!\n", iface, handled);
    if (handled) memset(handled, 0, sizeof(*handled));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NewWindowRequestedEventArgs_get_IsUserInitiated(ICoreWebView2NewWindowRequestedEventArgs3 *iface, BOOL *isUserInitiated)
{
    FIXME("iface %p, isUserInitiated %p stub!\n", iface, isUserInitiated);
    if (isUserInitiated) memset(isUserInitiated, 0, sizeof(*isUserInitiated));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NewWindowRequestedEventArgs_GetDeferral(ICoreWebView2NewWindowRequestedEventArgs3 *iface, ICoreWebView2Deferral **deferral)
{
    FIXME("iface %p, deferral %p stub!\n", iface, deferral);
    if (deferral) *deferral = stub_create(&stub_family_ICoreWebView2Deferral);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NewWindowRequestedEventArgs_get_WindowFeatures(ICoreWebView2NewWindowRequestedEventArgs3 *iface, ICoreWebView2WindowFeatures **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2WindowFeatures);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NewWindowRequestedEventArgs2_get_Name(ICoreWebView2NewWindowRequestedEventArgs3 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2NewWindowRequestedEventArgs3_get_OriginalSourceFrameInfo(ICoreWebView2NewWindowRequestedEventArgs3 *iface, ICoreWebView2FrameInfo **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2FrameInfo2);
    return S_OK;
}

static const ICoreWebView2NewWindowRequestedEventArgs3Vtbl ICoreWebView2NewWindowRequestedEventArgs3_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2NewWindowRequestedEventArgs_get_Uri,
    ICoreWebView2NewWindowRequestedEventArgs_put_NewWindow,
    ICoreWebView2NewWindowRequestedEventArgs_get_NewWindow,
    ICoreWebView2NewWindowRequestedEventArgs_put_Handled,
    ICoreWebView2NewWindowRequestedEventArgs_get_Handled,
    ICoreWebView2NewWindowRequestedEventArgs_get_IsUserInitiated,
    ICoreWebView2NewWindowRequestedEventArgs_GetDeferral,
    ICoreWebView2NewWindowRequestedEventArgs_get_WindowFeatures,
    ICoreWebView2NewWindowRequestedEventArgs2_get_Name,
    ICoreWebView2NewWindowRequestedEventArgs3_get_OriginalSourceFrameInfo,
};

const struct stub_family stub_family_ICoreWebView2NewWindowRequestedEventArgs3 =
{
    "ICoreWebView2NewWindowRequestedEventArgs3",
    &ICoreWebView2NewWindowRequestedEventArgs3_stub_vtbl,
    ICoreWebView2NewWindowRequestedEventArgs3_iids,
};

/* ===== ICoreWebView2NonClientRegionChangedEventArgs ===== */

static const IID *const ICoreWebView2NonClientRegionChangedEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2NonClientRegionChangedEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2NonClientRegionChangedEventArgs_get_RegionKind(ICoreWebView2NonClientRegionChangedEventArgs *iface, COREWEBVIEW2_NON_CLIENT_REGION_KIND *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2NonClientRegionChangedEventArgsVtbl ICoreWebView2NonClientRegionChangedEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2NonClientRegionChangedEventArgs_get_RegionKind,
};

const struct stub_family stub_family_ICoreWebView2NonClientRegionChangedEventArgs =
{
    "ICoreWebView2NonClientRegionChangedEventArgs",
    &ICoreWebView2NonClientRegionChangedEventArgs_stub_vtbl,
    ICoreWebView2NonClientRegionChangedEventArgs_iids,
};

/* ===== ICoreWebView2PermissionRequestedEventArgs3 ===== */

static const IID *const ICoreWebView2PermissionRequestedEventArgs3_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2PermissionRequestedEventArgs,
    &IID_ICoreWebView2PermissionRequestedEventArgs2,
    &IID_ICoreWebView2PermissionRequestedEventArgs3,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2PermissionRequestedEventArgs_get_Uri(ICoreWebView2PermissionRequestedEventArgs3 *iface, LPWSTR *uri)
{
    FIXME("iface %p, uri %p stub!\n", iface, uri);
    if (uri) *uri = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PermissionRequestedEventArgs_get_PermissionKind(ICoreWebView2PermissionRequestedEventArgs3 *iface, COREWEBVIEW2_PERMISSION_KIND *permissionKind)
{
    FIXME("iface %p, permissionKind %p stub!\n", iface, permissionKind);
    if (permissionKind) memset(permissionKind, 0, sizeof(*permissionKind));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PermissionRequestedEventArgs_get_IsUserInitiated(ICoreWebView2PermissionRequestedEventArgs3 *iface, BOOL *isUserInitiated)
{
    FIXME("iface %p, isUserInitiated %p stub!\n", iface, isUserInitiated);
    if (isUserInitiated) memset(isUserInitiated, 0, sizeof(*isUserInitiated));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PermissionRequestedEventArgs_get_State(ICoreWebView2PermissionRequestedEventArgs3 *iface, COREWEBVIEW2_PERMISSION_STATE *state)
{
    FIXME("iface %p, state %p stub!\n", iface, state);
    if (state) memset(state, 0, sizeof(*state));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PermissionRequestedEventArgs_put_State(ICoreWebView2PermissionRequestedEventArgs3 *iface, COREWEBVIEW2_PERMISSION_STATE state)
{
    FIXME("iface %p, state %d stub!\n", iface, (int)state);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PermissionRequestedEventArgs_GetDeferral(ICoreWebView2PermissionRequestedEventArgs3 *iface, ICoreWebView2Deferral **deferral)
{
    FIXME("iface %p, deferral %p stub!\n", iface, deferral);
    if (deferral) *deferral = stub_create(&stub_family_ICoreWebView2Deferral);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PermissionRequestedEventArgs2_get_Handled(ICoreWebView2PermissionRequestedEventArgs3 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PermissionRequestedEventArgs2_put_Handled(ICoreWebView2PermissionRequestedEventArgs3 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PermissionRequestedEventArgs3_get_SavesInProfile(ICoreWebView2PermissionRequestedEventArgs3 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PermissionRequestedEventArgs3_put_SavesInProfile(ICoreWebView2PermissionRequestedEventArgs3 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static const ICoreWebView2PermissionRequestedEventArgs3Vtbl ICoreWebView2PermissionRequestedEventArgs3_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2PermissionRequestedEventArgs_get_Uri,
    ICoreWebView2PermissionRequestedEventArgs_get_PermissionKind,
    ICoreWebView2PermissionRequestedEventArgs_get_IsUserInitiated,
    ICoreWebView2PermissionRequestedEventArgs_get_State,
    ICoreWebView2PermissionRequestedEventArgs_put_State,
    ICoreWebView2PermissionRequestedEventArgs_GetDeferral,
    ICoreWebView2PermissionRequestedEventArgs2_get_Handled,
    ICoreWebView2PermissionRequestedEventArgs2_put_Handled,
    ICoreWebView2PermissionRequestedEventArgs3_get_SavesInProfile,
    ICoreWebView2PermissionRequestedEventArgs3_put_SavesInProfile,
};

const struct stub_family stub_family_ICoreWebView2PermissionRequestedEventArgs3 =
{
    "ICoreWebView2PermissionRequestedEventArgs3",
    &ICoreWebView2PermissionRequestedEventArgs3_stub_vtbl,
    ICoreWebView2PermissionRequestedEventArgs3_iids,
};

/* ===== ICoreWebView2PermissionSetting ===== */

static const IID *const ICoreWebView2PermissionSetting_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2PermissionSetting,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2PermissionSetting_get_PermissionKind(ICoreWebView2PermissionSetting *iface, COREWEBVIEW2_PERMISSION_KIND *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PermissionSetting_get_PermissionOrigin(ICoreWebView2PermissionSetting *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2PermissionSetting_get_PermissionState(ICoreWebView2PermissionSetting *iface, COREWEBVIEW2_PERMISSION_STATE *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2PermissionSettingVtbl ICoreWebView2PermissionSetting_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2PermissionSetting_get_PermissionKind,
    ICoreWebView2PermissionSetting_get_PermissionOrigin,
    ICoreWebView2PermissionSetting_get_PermissionState,
};

const struct stub_family stub_family_ICoreWebView2PermissionSetting =
{
    "ICoreWebView2PermissionSetting",
    &ICoreWebView2PermissionSetting_stub_vtbl,
    ICoreWebView2PermissionSetting_iids,
};

/* ===== ICoreWebView2ProcessFailedEventArgs4 ===== */

static const IID *const ICoreWebView2ProcessFailedEventArgs4_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ProcessFailedEventArgs,
    &IID_ICoreWebView2ProcessFailedEventArgs2,
    &IID_ICoreWebView2ProcessFailedEventArgs3,
    &IID_ICoreWebView2ProcessFailedEventArgs4,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ProcessFailedEventArgs_get_ProcessFailedKind(ICoreWebView2ProcessFailedEventArgs4 *iface, COREWEBVIEW2_PROCESS_FAILED_KIND *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ProcessFailedEventArgs2_get_Reason(ICoreWebView2ProcessFailedEventArgs4 *iface, COREWEBVIEW2_PROCESS_FAILED_REASON *reason)
{
    FIXME("iface %p, reason %p stub!\n", iface, reason);
    if (reason) memset(reason, 0, sizeof(*reason));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ProcessFailedEventArgs2_get_ExitCode(ICoreWebView2ProcessFailedEventArgs4 *iface, int *exitCode)
{
    FIXME("iface %p, exitCode %p stub!\n", iface, exitCode);
    if (exitCode) memset(exitCode, 0, sizeof(*exitCode));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ProcessFailedEventArgs2_get_ProcessDescription(ICoreWebView2ProcessFailedEventArgs4 *iface, LPWSTR *processDescription)
{
    FIXME("iface %p, processDescription %p stub!\n", iface, processDescription);
    if (processDescription) *processDescription = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ProcessFailedEventArgs2_get_FrameInfosForFailedProcess(ICoreWebView2ProcessFailedEventArgs4 *iface, ICoreWebView2FrameInfoCollection **frames)
{
    FIXME("iface %p, frames %p stub!\n", iface, frames);
    if (frames) *frames = stub_create(&stub_family_ICoreWebView2FrameInfoCollection);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ProcessFailedEventArgs3_get_FailureSourceModulePath(ICoreWebView2ProcessFailedEventArgs4 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ProcessFailedEventArgs4_get_CrashReport(ICoreWebView2ProcessFailedEventArgs4 *iface, ICoreWebView2CrashReport **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2CrashReport);
    return S_OK;
}

static const ICoreWebView2ProcessFailedEventArgs4Vtbl ICoreWebView2ProcessFailedEventArgs4_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ProcessFailedEventArgs_get_ProcessFailedKind,
    ICoreWebView2ProcessFailedEventArgs2_get_Reason,
    ICoreWebView2ProcessFailedEventArgs2_get_ExitCode,
    ICoreWebView2ProcessFailedEventArgs2_get_ProcessDescription,
    ICoreWebView2ProcessFailedEventArgs2_get_FrameInfosForFailedProcess,
    ICoreWebView2ProcessFailedEventArgs3_get_FailureSourceModulePath,
    ICoreWebView2ProcessFailedEventArgs4_get_CrashReport,
};

const struct stub_family stub_family_ICoreWebView2ProcessFailedEventArgs4 =
{
    "ICoreWebView2ProcessFailedEventArgs4",
    &ICoreWebView2ProcessFailedEventArgs4_stub_vtbl,
    ICoreWebView2ProcessFailedEventArgs4_iids,
};

/* ===== ICoreWebView2Profile9 ===== */

static const IID *const ICoreWebView2Profile9_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2Profile,
    &IID_ICoreWebView2Profile2,
    &IID_ICoreWebView2Profile3,
    &IID_ICoreWebView2Profile4,
    &IID_ICoreWebView2Profile5,
    &IID_ICoreWebView2Profile6,
    &IID_ICoreWebView2Profile7,
    &IID_ICoreWebView2Profile8,
    &IID_ICoreWebView2Profile9,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile_get_ProfileName(ICoreWebView2Profile9 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile_get_IsInPrivateModeEnabled(ICoreWebView2Profile9 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile_get_ProfilePath(ICoreWebView2Profile9 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile_get_DefaultDownloadFolderPath(ICoreWebView2Profile9 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile_put_DefaultDownloadFolderPath(ICoreWebView2Profile9 *iface, LPCWSTR value)
{
    FIXME("iface %p, value %s stub!\n", iface, debugstr_w(value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile_get_PreferredColorScheme(ICoreWebView2Profile9 *iface, COREWEBVIEW2_PREFERRED_COLOR_SCHEME *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile_put_PreferredColorScheme(ICoreWebView2Profile9 *iface, COREWEBVIEW2_PREFERRED_COLOR_SCHEME value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile2_ClearBrowsingData(ICoreWebView2Profile9 *iface, COREWEBVIEW2_BROWSING_DATA_KINDS dataKinds, ICoreWebView2ClearBrowsingDataCompletedHandler *handler)
{
    FIXME("iface %p, dataKinds %d, handler %p stub!\n", iface, (int)dataKinds, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2ClearBrowsingDataCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile2_ClearBrowsingDataInTimeRange(ICoreWebView2Profile9 *iface, COREWEBVIEW2_BROWSING_DATA_KINDS dataKinds, double startTime, double endTime, ICoreWebView2ClearBrowsingDataCompletedHandler *handler)
{
    FIXME("iface %p, dataKinds %d, startTime %f, endTime %f, handler %p stub!\n", iface, (int)dataKinds, startTime, endTime, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2ClearBrowsingDataCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile2_ClearBrowsingDataAll(ICoreWebView2Profile9 *iface, ICoreWebView2ClearBrowsingDataCompletedHandler *handler)
{
    FIXME("iface %p, handler %p stub!\n", iface, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2ClearBrowsingDataCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile3_get_PreferredTrackingPreventionLevel(ICoreWebView2Profile9 *iface, COREWEBVIEW2_TRACKING_PREVENTION_LEVEL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile3_put_PreferredTrackingPreventionLevel(ICoreWebView2Profile9 *iface, COREWEBVIEW2_TRACKING_PREVENTION_LEVEL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile4_SetPermissionState(ICoreWebView2Profile9 *iface, COREWEBVIEW2_PERMISSION_KIND PermissionKind, LPCWSTR origin, COREWEBVIEW2_PERMISSION_STATE State, ICoreWebView2SetPermissionStateCompletedHandler *handler)
{
    FIXME("iface %p, PermissionKind %d, origin %s, State %d, handler %p stub!\n", iface, (int)PermissionKind, debugstr_w(origin), (int)State, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2SetPermissionStateCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile4_GetNonDefaultPermissionSettings(ICoreWebView2Profile9 *iface, ICoreWebView2GetNonDefaultPermissionSettingsCompletedHandler *handler)
{
    FIXME("iface %p, handler %p stub!\n", iface, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2GetNonDefaultPermissionSettingsCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile5_get_CookieManager(ICoreWebView2Profile9 *iface, ICoreWebView2CookieManager **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2CookieManager);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile6_get_IsPasswordAutosaveEnabled(ICoreWebView2Profile9 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile6_put_IsPasswordAutosaveEnabled(ICoreWebView2Profile9 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile6_get_IsGeneralAutofillEnabled(ICoreWebView2Profile9 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile6_put_IsGeneralAutofillEnabled(ICoreWebView2Profile9 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile7_AddBrowserExtension(ICoreWebView2Profile9 *iface, LPCWSTR extensionFolderPath, ICoreWebView2ProfileAddBrowserExtensionCompletedHandler *handler)
{
    FIXME("iface %p, extensionFolderPath %s, handler %p stub!\n", iface, debugstr_w(extensionFolderPath), (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2ProfileAddBrowserExtensionCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile7_GetBrowserExtensions(ICoreWebView2Profile9 *iface, ICoreWebView2ProfileGetBrowserExtensionsCompletedHandler *handler)
{
    FIXME("iface %p, handler %p stub!\n", iface, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2ProfileGetBrowserExtensionsCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile8_Delete(ICoreWebView2Profile9 *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile8_add_Deleted(ICoreWebView2Profile9 *iface, ICoreWebView2ProfileDeletedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile8_remove_Deleted(ICoreWebView2Profile9 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile9_get_AreWebViewScriptApisEnabledForServiceWorkers(ICoreWebView2Profile9 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile9_put_AreWebViewScriptApisEnabledForServiceWorkers(ICoreWebView2Profile9 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile9_get_ServiceWorkerManager(ICoreWebView2Profile9 *iface, ICoreWebView2ServiceWorkerManager **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ServiceWorkerManager);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Profile9_get_SharedWorkerManager(ICoreWebView2Profile9 *iface, ICoreWebView2SharedWorkerManager **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2SharedWorkerManager);
    return S_OK;
}

static const ICoreWebView2Profile9Vtbl ICoreWebView2Profile9_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2Profile_get_ProfileName,
    ICoreWebView2Profile_get_IsInPrivateModeEnabled,
    ICoreWebView2Profile_get_ProfilePath,
    ICoreWebView2Profile_get_DefaultDownloadFolderPath,
    ICoreWebView2Profile_put_DefaultDownloadFolderPath,
    ICoreWebView2Profile_get_PreferredColorScheme,
    ICoreWebView2Profile_put_PreferredColorScheme,
    ICoreWebView2Profile2_ClearBrowsingData,
    ICoreWebView2Profile2_ClearBrowsingDataInTimeRange,
    ICoreWebView2Profile2_ClearBrowsingDataAll,
    ICoreWebView2Profile3_get_PreferredTrackingPreventionLevel,
    ICoreWebView2Profile3_put_PreferredTrackingPreventionLevel,
    ICoreWebView2Profile4_SetPermissionState,
    ICoreWebView2Profile4_GetNonDefaultPermissionSettings,
    ICoreWebView2Profile5_get_CookieManager,
    ICoreWebView2Profile6_get_IsPasswordAutosaveEnabled,
    ICoreWebView2Profile6_put_IsPasswordAutosaveEnabled,
    ICoreWebView2Profile6_get_IsGeneralAutofillEnabled,
    ICoreWebView2Profile6_put_IsGeneralAutofillEnabled,
    ICoreWebView2Profile7_AddBrowserExtension,
    ICoreWebView2Profile7_GetBrowserExtensions,
    ICoreWebView2Profile8_Delete,
    ICoreWebView2Profile8_add_Deleted,
    ICoreWebView2Profile8_remove_Deleted,
    ICoreWebView2Profile9_get_AreWebViewScriptApisEnabledForServiceWorkers,
    ICoreWebView2Profile9_put_AreWebViewScriptApisEnabledForServiceWorkers,
    ICoreWebView2Profile9_get_ServiceWorkerManager,
    ICoreWebView2Profile9_get_SharedWorkerManager,
};

const struct stub_family stub_family_ICoreWebView2Profile9 =
{
    "ICoreWebView2Profile9",
    &ICoreWebView2Profile9_stub_vtbl,
    ICoreWebView2Profile9_iids,
};

/* ===== ICoreWebView2BrowserExtensionList ===== */

static const IID *const ICoreWebView2BrowserExtensionList_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2BrowserExtensionList,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2BrowserExtensionList_get_Count(ICoreWebView2BrowserExtensionList *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2BrowserExtensionList_GetValueAtIndex(ICoreWebView2BrowserExtensionList *iface, UINT32 index, ICoreWebView2BrowserExtension **value)
{
    FIXME("iface %p, index %d, value %p stub!\n", iface, (int)index, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2BrowserExtension);
    return S_OK;
}

static const ICoreWebView2BrowserExtensionListVtbl ICoreWebView2BrowserExtensionList_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2BrowserExtensionList_get_Count,
    ICoreWebView2BrowserExtensionList_GetValueAtIndex,
};

const struct stub_family stub_family_ICoreWebView2BrowserExtensionList =
{
    "ICoreWebView2BrowserExtensionList",
    &ICoreWebView2BrowserExtensionList_stub_vtbl,
    ICoreWebView2BrowserExtensionList_iids,
};

/* ===== ICoreWebView2SaveFileSecurityCheckStartingEventArgs ===== */

static const IID *const ICoreWebView2SaveFileSecurityCheckStartingEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2SaveFileSecurityCheckStartingEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveFileSecurityCheckStartingEventArgs_get_CancelSave(ICoreWebView2SaveFileSecurityCheckStartingEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveFileSecurityCheckStartingEventArgs_put_CancelSave(ICoreWebView2SaveFileSecurityCheckStartingEventArgs *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveFileSecurityCheckStartingEventArgs_get_DocumentOriginUri(ICoreWebView2SaveFileSecurityCheckStartingEventArgs *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveFileSecurityCheckStartingEventArgs_get_FileExtension(ICoreWebView2SaveFileSecurityCheckStartingEventArgs *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveFileSecurityCheckStartingEventArgs_get_FilePath(ICoreWebView2SaveFileSecurityCheckStartingEventArgs *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveFileSecurityCheckStartingEventArgs_get_SuppressDefaultPolicy(ICoreWebView2SaveFileSecurityCheckStartingEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveFileSecurityCheckStartingEventArgs_put_SuppressDefaultPolicy(ICoreWebView2SaveFileSecurityCheckStartingEventArgs *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SaveFileSecurityCheckStartingEventArgs_GetDeferral(ICoreWebView2SaveFileSecurityCheckStartingEventArgs *iface, ICoreWebView2Deferral **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2Deferral);
    return S_OK;
}

static const ICoreWebView2SaveFileSecurityCheckStartingEventArgsVtbl ICoreWebView2SaveFileSecurityCheckStartingEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2SaveFileSecurityCheckStartingEventArgs_get_CancelSave,
    ICoreWebView2SaveFileSecurityCheckStartingEventArgs_put_CancelSave,
    ICoreWebView2SaveFileSecurityCheckStartingEventArgs_get_DocumentOriginUri,
    ICoreWebView2SaveFileSecurityCheckStartingEventArgs_get_FileExtension,
    ICoreWebView2SaveFileSecurityCheckStartingEventArgs_get_FilePath,
    ICoreWebView2SaveFileSecurityCheckStartingEventArgs_get_SuppressDefaultPolicy,
    ICoreWebView2SaveFileSecurityCheckStartingEventArgs_put_SuppressDefaultPolicy,
    ICoreWebView2SaveFileSecurityCheckStartingEventArgs_GetDeferral,
};

const struct stub_family stub_family_ICoreWebView2SaveFileSecurityCheckStartingEventArgs =
{
    "ICoreWebView2SaveFileSecurityCheckStartingEventArgs",
    &ICoreWebView2SaveFileSecurityCheckStartingEventArgs_stub_vtbl,
    ICoreWebView2SaveFileSecurityCheckStartingEventArgs_iids,
};

/* ===== ICoreWebView2ScreenCaptureStartingEventArgs ===== */

static const IID *const ICoreWebView2ScreenCaptureStartingEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ScreenCaptureStartingEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScreenCaptureStartingEventArgs_get_Cancel(ICoreWebView2ScreenCaptureStartingEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScreenCaptureStartingEventArgs_put_Cancel(ICoreWebView2ScreenCaptureStartingEventArgs *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScreenCaptureStartingEventArgs_get_Handled(ICoreWebView2ScreenCaptureStartingEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScreenCaptureStartingEventArgs_put_Handled(ICoreWebView2ScreenCaptureStartingEventArgs *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScreenCaptureStartingEventArgs_get_OriginalSourceFrameInfo(ICoreWebView2ScreenCaptureStartingEventArgs *iface, ICoreWebView2FrameInfo **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2FrameInfo2);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ScreenCaptureStartingEventArgs_GetDeferral(ICoreWebView2ScreenCaptureStartingEventArgs *iface, ICoreWebView2Deferral **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2Deferral);
    return S_OK;
}

static const ICoreWebView2ScreenCaptureStartingEventArgsVtbl ICoreWebView2ScreenCaptureStartingEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ScreenCaptureStartingEventArgs_get_Cancel,
    ICoreWebView2ScreenCaptureStartingEventArgs_put_Cancel,
    ICoreWebView2ScreenCaptureStartingEventArgs_get_Handled,
    ICoreWebView2ScreenCaptureStartingEventArgs_put_Handled,
    ICoreWebView2ScreenCaptureStartingEventArgs_get_OriginalSourceFrameInfo,
    ICoreWebView2ScreenCaptureStartingEventArgs_GetDeferral,
};

const struct stub_family stub_family_ICoreWebView2ScreenCaptureStartingEventArgs =
{
    "ICoreWebView2ScreenCaptureStartingEventArgs",
    &ICoreWebView2ScreenCaptureStartingEventArgs_stub_vtbl,
    ICoreWebView2ScreenCaptureStartingEventArgs_iids,
};

/* ===== ICoreWebView2ServiceWorker ===== */

static const IID *const ICoreWebView2ServiceWorker_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ServiceWorker,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorker_get_ScriptUri(ICoreWebView2ServiceWorker *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorker_add_Destroying(ICoreWebView2ServiceWorker *iface, ICoreWebView2ServiceWorkerDestroyingEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorker_remove_Destroying(ICoreWebView2ServiceWorker *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorker_add_WebMessageReceived(ICoreWebView2ServiceWorker *iface, ICoreWebView2ServiceWorkerWebMessageReceivedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorker_remove_WebMessageReceived(ICoreWebView2ServiceWorker *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorker_PostWebMessageAsJson(ICoreWebView2ServiceWorker *iface, LPCWSTR webMessageAsJson)
{
    FIXME("iface %p, webMessageAsJson %s stub!\n", iface, debugstr_w(webMessageAsJson));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorker_PostWebMessageAsString(ICoreWebView2ServiceWorker *iface, LPCWSTR webMessageAsString)
{
    FIXME("iface %p, webMessageAsString %s stub!\n", iface, debugstr_w(webMessageAsString));
    return S_OK;
}

static const ICoreWebView2ServiceWorkerVtbl ICoreWebView2ServiceWorker_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ServiceWorker_get_ScriptUri,
    ICoreWebView2ServiceWorker_add_Destroying,
    ICoreWebView2ServiceWorker_remove_Destroying,
    ICoreWebView2ServiceWorker_add_WebMessageReceived,
    ICoreWebView2ServiceWorker_remove_WebMessageReceived,
    ICoreWebView2ServiceWorker_PostWebMessageAsJson,
    ICoreWebView2ServiceWorker_PostWebMessageAsString,
};

const struct stub_family stub_family_ICoreWebView2ServiceWorker =
{
    "ICoreWebView2ServiceWorker",
    &ICoreWebView2ServiceWorker_stub_vtbl,
    ICoreWebView2ServiceWorker_iids,
};

/* ===== ICoreWebView2ServiceWorkerActivatedEventArgs ===== */

static const IID *const ICoreWebView2ServiceWorkerActivatedEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ServiceWorkerActivatedEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorkerActivatedEventArgs_get_ActiveServiceWorker(ICoreWebView2ServiceWorkerActivatedEventArgs *iface, ICoreWebView2ServiceWorker **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ServiceWorker);
    return S_OK;
}

static const ICoreWebView2ServiceWorkerActivatedEventArgsVtbl ICoreWebView2ServiceWorkerActivatedEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ServiceWorkerActivatedEventArgs_get_ActiveServiceWorker,
};

const struct stub_family stub_family_ICoreWebView2ServiceWorkerActivatedEventArgs =
{
    "ICoreWebView2ServiceWorkerActivatedEventArgs",
    &ICoreWebView2ServiceWorkerActivatedEventArgs_stub_vtbl,
    ICoreWebView2ServiceWorkerActivatedEventArgs_iids,
};

/* ===== ICoreWebView2ServiceWorkerRegistrationCollectionView ===== */

static const IID *const ICoreWebView2ServiceWorkerRegistrationCollectionView_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ServiceWorkerRegistrationCollectionView,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorkerRegistrationCollectionView_get_Count(ICoreWebView2ServiceWorkerRegistrationCollectionView *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorkerRegistrationCollectionView_GetValueAtIndex(ICoreWebView2ServiceWorkerRegistrationCollectionView *iface, UINT32 index, ICoreWebView2ServiceWorkerRegistration **value)
{
    FIXME("iface %p, index %d, value %p stub!\n", iface, (int)index, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ServiceWorkerRegistration);
    return S_OK;
}

static const ICoreWebView2ServiceWorkerRegistrationCollectionViewVtbl ICoreWebView2ServiceWorkerRegistrationCollectionView_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ServiceWorkerRegistrationCollectionView_get_Count,
    ICoreWebView2ServiceWorkerRegistrationCollectionView_GetValueAtIndex,
};

const struct stub_family stub_family_ICoreWebView2ServiceWorkerRegistrationCollectionView =
{
    "ICoreWebView2ServiceWorkerRegistrationCollectionView",
    &ICoreWebView2ServiceWorkerRegistrationCollectionView_stub_vtbl,
    ICoreWebView2ServiceWorkerRegistrationCollectionView_iids,
};

/* ===== ICoreWebView2ServiceWorkerManager ===== */

static const IID *const ICoreWebView2ServiceWorkerManager_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ServiceWorkerManager,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorkerManager_add_ServiceWorkerRegistered(ICoreWebView2ServiceWorkerManager *iface, ICoreWebView2ServiceWorkerRegisteredEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorkerManager_remove_ServiceWorkerRegistered(ICoreWebView2ServiceWorkerManager *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorkerManager_GetServiceWorkerRegistrations(ICoreWebView2ServiceWorkerManager *iface, ICoreWebView2GetServiceWorkerRegistrationsCompletedHandler *handler)
{
    FIXME("iface %p, handler %p stub!\n", iface, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2GetServiceWorkerRegistrationsCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorkerManager_GetServiceWorkerRegistrationsForScope(ICoreWebView2ServiceWorkerManager *iface, LPCWSTR scopeUri, ICoreWebView2GetServiceWorkerRegistrationsCompletedHandler *handler)
{
    FIXME("iface %p, scopeUri %s, handler %p stub!\n", iface, debugstr_w(scopeUri), (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2GetServiceWorkerRegistrationsCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static const ICoreWebView2ServiceWorkerManagerVtbl ICoreWebView2ServiceWorkerManager_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ServiceWorkerManager_add_ServiceWorkerRegistered,
    ICoreWebView2ServiceWorkerManager_remove_ServiceWorkerRegistered,
    ICoreWebView2ServiceWorkerManager_GetServiceWorkerRegistrations,
    ICoreWebView2ServiceWorkerManager_GetServiceWorkerRegistrationsForScope,
};

const struct stub_family stub_family_ICoreWebView2ServiceWorkerManager =
{
    "ICoreWebView2ServiceWorkerManager",
    &ICoreWebView2ServiceWorkerManager_stub_vtbl,
    ICoreWebView2ServiceWorkerManager_iids,
};

/* ===== ICoreWebView2ServiceWorkerRegisteredEventArgs ===== */

static const IID *const ICoreWebView2ServiceWorkerRegisteredEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ServiceWorkerRegisteredEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorkerRegisteredEventArgs_get_ServiceWorkerRegistration(ICoreWebView2ServiceWorkerRegisteredEventArgs *iface, ICoreWebView2ServiceWorkerRegistration **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ServiceWorkerRegistration);
    return S_OK;
}

static const ICoreWebView2ServiceWorkerRegisteredEventArgsVtbl ICoreWebView2ServiceWorkerRegisteredEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ServiceWorkerRegisteredEventArgs_get_ServiceWorkerRegistration,
};

const struct stub_family stub_family_ICoreWebView2ServiceWorkerRegisteredEventArgs =
{
    "ICoreWebView2ServiceWorkerRegisteredEventArgs",
    &ICoreWebView2ServiceWorkerRegisteredEventArgs_stub_vtbl,
    ICoreWebView2ServiceWorkerRegisteredEventArgs_iids,
};

/* ===== ICoreWebView2ServiceWorkerRegistration ===== */

static const IID *const ICoreWebView2ServiceWorkerRegistration_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2ServiceWorkerRegistration,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorkerRegistration_get_ActiveServiceWorker(ICoreWebView2ServiceWorkerRegistration *iface, ICoreWebView2ServiceWorker **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ServiceWorker);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorkerRegistration_get_Origin(ICoreWebView2ServiceWorkerRegistration *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorkerRegistration_get_ScopeUri(ICoreWebView2ServiceWorkerRegistration *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorkerRegistration_get_TopLevelOrigin(ICoreWebView2ServiceWorkerRegistration *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorkerRegistration_add_ServiceWorkerActivated(ICoreWebView2ServiceWorkerRegistration *iface, ICoreWebView2ServiceWorkerActivatedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorkerRegistration_remove_ServiceWorkerActivated(ICoreWebView2ServiceWorkerRegistration *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorkerRegistration_add_Unregistering(ICoreWebView2ServiceWorkerRegistration *iface, ICoreWebView2ServiceWorkerRegistrationUnregisteringEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2ServiceWorkerRegistration_remove_Unregistering(ICoreWebView2ServiceWorkerRegistration *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static const ICoreWebView2ServiceWorkerRegistrationVtbl ICoreWebView2ServiceWorkerRegistration_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2ServiceWorkerRegistration_get_ActiveServiceWorker,
    ICoreWebView2ServiceWorkerRegistration_get_Origin,
    ICoreWebView2ServiceWorkerRegistration_get_ScopeUri,
    ICoreWebView2ServiceWorkerRegistration_get_TopLevelOrigin,
    ICoreWebView2ServiceWorkerRegistration_add_ServiceWorkerActivated,
    ICoreWebView2ServiceWorkerRegistration_remove_ServiceWorkerActivated,
    ICoreWebView2ServiceWorkerRegistration_add_Unregistering,
    ICoreWebView2ServiceWorkerRegistration_remove_Unregistering,
};

const struct stub_family stub_family_ICoreWebView2ServiceWorkerRegistration =
{
    "ICoreWebView2ServiceWorkerRegistration",
    &ICoreWebView2ServiceWorkerRegistration_stub_vtbl,
    ICoreWebView2ServiceWorkerRegistration_iids,
};

/* ===== ICoreWebView2Settings9 ===== */

static const IID *const ICoreWebView2Settings9_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2Settings,
    &IID_ICoreWebView2Settings2,
    &IID_ICoreWebView2Settings3,
    &IID_ICoreWebView2Settings4,
    &IID_ICoreWebView2Settings5,
    &IID_ICoreWebView2Settings6,
    &IID_ICoreWebView2Settings7,
    &IID_ICoreWebView2Settings8,
    &IID_ICoreWebView2Settings9,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_get_IsScriptEnabled(ICoreWebView2Settings9 *iface, BOOL *isScriptEnabled)
{
    FIXME("iface %p, isScriptEnabled %p stub!\n", iface, isScriptEnabled);
    if (isScriptEnabled) memset(isScriptEnabled, 0, sizeof(*isScriptEnabled));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_put_IsScriptEnabled(ICoreWebView2Settings9 *iface, BOOL isScriptEnabled)
{
    FIXME("iface %p, isScriptEnabled %d stub!\n", iface, (int)isScriptEnabled);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_get_IsWebMessageEnabled(ICoreWebView2Settings9 *iface, BOOL *isWebMessageEnabled)
{
    FIXME("iface %p, isWebMessageEnabled %p stub!\n", iface, isWebMessageEnabled);
    if (isWebMessageEnabled) memset(isWebMessageEnabled, 0, sizeof(*isWebMessageEnabled));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_put_IsWebMessageEnabled(ICoreWebView2Settings9 *iface, BOOL isWebMessageEnabled)
{
    FIXME("iface %p, isWebMessageEnabled %d stub!\n", iface, (int)isWebMessageEnabled);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_get_AreDefaultScriptDialogsEnabled(ICoreWebView2Settings9 *iface, BOOL *areDefaultScriptDialogsEnabled)
{
    FIXME("iface %p, areDefaultScriptDialogsEnabled %p stub!\n", iface, areDefaultScriptDialogsEnabled);
    if (areDefaultScriptDialogsEnabled) memset(areDefaultScriptDialogsEnabled, 0, sizeof(*areDefaultScriptDialogsEnabled));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_put_AreDefaultScriptDialogsEnabled(ICoreWebView2Settings9 *iface, BOOL areDefaultScriptDialogsEnabled)
{
    FIXME("iface %p, areDefaultScriptDialogsEnabled %d stub!\n", iface, (int)areDefaultScriptDialogsEnabled);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_get_IsStatusBarEnabled(ICoreWebView2Settings9 *iface, BOOL *isStatusBarEnabled)
{
    FIXME("iface %p, isStatusBarEnabled %p stub!\n", iface, isStatusBarEnabled);
    if (isStatusBarEnabled) memset(isStatusBarEnabled, 0, sizeof(*isStatusBarEnabled));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_put_IsStatusBarEnabled(ICoreWebView2Settings9 *iface, BOOL isStatusBarEnabled)
{
    FIXME("iface %p, isStatusBarEnabled %d stub!\n", iface, (int)isStatusBarEnabled);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_get_AreDevToolsEnabled(ICoreWebView2Settings9 *iface, BOOL *areDevToolsEnabled)
{
    FIXME("iface %p, areDevToolsEnabled %p stub!\n", iface, areDevToolsEnabled);
    if (areDevToolsEnabled) memset(areDevToolsEnabled, 0, sizeof(*areDevToolsEnabled));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_put_AreDevToolsEnabled(ICoreWebView2Settings9 *iface, BOOL areDevToolsEnabled)
{
    FIXME("iface %p, areDevToolsEnabled %d stub!\n", iface, (int)areDevToolsEnabled);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_get_AreDefaultContextMenusEnabled(ICoreWebView2Settings9 *iface, BOOL *enabled)
{
    FIXME("iface %p, enabled %p stub!\n", iface, enabled);
    if (enabled) memset(enabled, 0, sizeof(*enabled));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_put_AreDefaultContextMenusEnabled(ICoreWebView2Settings9 *iface, BOOL enabled)
{
    FIXME("iface %p, enabled %d stub!\n", iface, (int)enabled);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_get_AreHostObjectsAllowed(ICoreWebView2Settings9 *iface, BOOL *allowed)
{
    FIXME("iface %p, allowed %p stub!\n", iface, allowed);
    if (allowed) memset(allowed, 0, sizeof(*allowed));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_put_AreHostObjectsAllowed(ICoreWebView2Settings9 *iface, BOOL allowed)
{
    FIXME("iface %p, allowed %d stub!\n", iface, (int)allowed);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_get_IsZoomControlEnabled(ICoreWebView2Settings9 *iface, BOOL *enabled)
{
    FIXME("iface %p, enabled %p stub!\n", iface, enabled);
    if (enabled) memset(enabled, 0, sizeof(*enabled));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_put_IsZoomControlEnabled(ICoreWebView2Settings9 *iface, BOOL enabled)
{
    FIXME("iface %p, enabled %d stub!\n", iface, (int)enabled);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_get_IsBuiltInErrorPageEnabled(ICoreWebView2Settings9 *iface, BOOL *enabled)
{
    FIXME("iface %p, enabled %p stub!\n", iface, enabled);
    if (enabled) memset(enabled, 0, sizeof(*enabled));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings_put_IsBuiltInErrorPageEnabled(ICoreWebView2Settings9 *iface, BOOL enabled)
{
    FIXME("iface %p, enabled %d stub!\n", iface, (int)enabled);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings2_get_UserAgent(ICoreWebView2Settings9 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings2_put_UserAgent(ICoreWebView2Settings9 *iface, LPCWSTR value)
{
    FIXME("iface %p, value %s stub!\n", iface, debugstr_w(value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings3_get_AreBrowserAcceleratorKeysEnabled(ICoreWebView2Settings9 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings3_put_AreBrowserAcceleratorKeysEnabled(ICoreWebView2Settings9 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings4_get_IsPasswordAutosaveEnabled(ICoreWebView2Settings9 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings4_put_IsPasswordAutosaveEnabled(ICoreWebView2Settings9 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings4_get_IsGeneralAutofillEnabled(ICoreWebView2Settings9 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings4_put_IsGeneralAutofillEnabled(ICoreWebView2Settings9 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings5_get_IsPinchZoomEnabled(ICoreWebView2Settings9 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings5_put_IsPinchZoomEnabled(ICoreWebView2Settings9 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings6_get_IsSwipeNavigationEnabled(ICoreWebView2Settings9 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings6_put_IsSwipeNavigationEnabled(ICoreWebView2Settings9 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings7_get_HiddenPdfToolbarItems(ICoreWebView2Settings9 *iface, COREWEBVIEW2_PDF_TOOLBAR_ITEMS *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings7_put_HiddenPdfToolbarItems(ICoreWebView2Settings9 *iface, COREWEBVIEW2_PDF_TOOLBAR_ITEMS value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings8_get_IsReputationCheckingRequired(ICoreWebView2Settings9 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings8_put_IsReputationCheckingRequired(ICoreWebView2Settings9 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings9_get_IsNonClientRegionSupportEnabled(ICoreWebView2Settings9 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2Settings9_put_IsNonClientRegionSupportEnabled(ICoreWebView2Settings9 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static const ICoreWebView2Settings9Vtbl ICoreWebView2Settings9_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2Settings_get_IsScriptEnabled,
    ICoreWebView2Settings_put_IsScriptEnabled,
    ICoreWebView2Settings_get_IsWebMessageEnabled,
    ICoreWebView2Settings_put_IsWebMessageEnabled,
    ICoreWebView2Settings_get_AreDefaultScriptDialogsEnabled,
    ICoreWebView2Settings_put_AreDefaultScriptDialogsEnabled,
    ICoreWebView2Settings_get_IsStatusBarEnabled,
    ICoreWebView2Settings_put_IsStatusBarEnabled,
    ICoreWebView2Settings_get_AreDevToolsEnabled,
    ICoreWebView2Settings_put_AreDevToolsEnabled,
    ICoreWebView2Settings_get_AreDefaultContextMenusEnabled,
    ICoreWebView2Settings_put_AreDefaultContextMenusEnabled,
    ICoreWebView2Settings_get_AreHostObjectsAllowed,
    ICoreWebView2Settings_put_AreHostObjectsAllowed,
    ICoreWebView2Settings_get_IsZoomControlEnabled,
    ICoreWebView2Settings_put_IsZoomControlEnabled,
    ICoreWebView2Settings_get_IsBuiltInErrorPageEnabled,
    ICoreWebView2Settings_put_IsBuiltInErrorPageEnabled,
    ICoreWebView2Settings2_get_UserAgent,
    ICoreWebView2Settings2_put_UserAgent,
    ICoreWebView2Settings3_get_AreBrowserAcceleratorKeysEnabled,
    ICoreWebView2Settings3_put_AreBrowserAcceleratorKeysEnabled,
    ICoreWebView2Settings4_get_IsPasswordAutosaveEnabled,
    ICoreWebView2Settings4_put_IsPasswordAutosaveEnabled,
    ICoreWebView2Settings4_get_IsGeneralAutofillEnabled,
    ICoreWebView2Settings4_put_IsGeneralAutofillEnabled,
    ICoreWebView2Settings5_get_IsPinchZoomEnabled,
    ICoreWebView2Settings5_put_IsPinchZoomEnabled,
    ICoreWebView2Settings6_get_IsSwipeNavigationEnabled,
    ICoreWebView2Settings6_put_IsSwipeNavigationEnabled,
    ICoreWebView2Settings7_get_HiddenPdfToolbarItems,
    ICoreWebView2Settings7_put_HiddenPdfToolbarItems,
    ICoreWebView2Settings8_get_IsReputationCheckingRequired,
    ICoreWebView2Settings8_put_IsReputationCheckingRequired,
    ICoreWebView2Settings9_get_IsNonClientRegionSupportEnabled,
    ICoreWebView2Settings9_put_IsNonClientRegionSupportEnabled,
};

const struct stub_family stub_family_ICoreWebView2Settings9 =
{
    "ICoreWebView2Settings9",
    &ICoreWebView2Settings9_stub_vtbl,
    ICoreWebView2Settings9_iids,
};

/* ===== ICoreWebView2SharedWorker ===== */

static const IID *const ICoreWebView2SharedWorker_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2SharedWorker,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2SharedWorker_get_Origin(ICoreWebView2SharedWorker *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SharedWorker_get_ScriptUri(ICoreWebView2SharedWorker *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SharedWorker_get_TopLevelOrigin(ICoreWebView2SharedWorker *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SharedWorker_add_Destroying(ICoreWebView2SharedWorker *iface, ICoreWebView2SharedWorkerDestroyingEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SharedWorker_remove_Destroying(ICoreWebView2SharedWorker *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static const ICoreWebView2SharedWorkerVtbl ICoreWebView2SharedWorker_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2SharedWorker_get_Origin,
    ICoreWebView2SharedWorker_get_ScriptUri,
    ICoreWebView2SharedWorker_get_TopLevelOrigin,
    ICoreWebView2SharedWorker_add_Destroying,
    ICoreWebView2SharedWorker_remove_Destroying,
};

const struct stub_family stub_family_ICoreWebView2SharedWorker =
{
    "ICoreWebView2SharedWorker",
    &ICoreWebView2SharedWorker_stub_vtbl,
    ICoreWebView2SharedWorker_iids,
};

/* ===== ICoreWebView2SharedWorkerCreatedEventArgs ===== */

static const IID *const ICoreWebView2SharedWorkerCreatedEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2SharedWorkerCreatedEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2SharedWorkerCreatedEventArgs_get_Worker(ICoreWebView2SharedWorkerCreatedEventArgs *iface, ICoreWebView2SharedWorker **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2SharedWorker);
    return S_OK;
}

static const ICoreWebView2SharedWorkerCreatedEventArgsVtbl ICoreWebView2SharedWorkerCreatedEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2SharedWorkerCreatedEventArgs_get_Worker,
};

const struct stub_family stub_family_ICoreWebView2SharedWorkerCreatedEventArgs =
{
    "ICoreWebView2SharedWorkerCreatedEventArgs",
    &ICoreWebView2SharedWorkerCreatedEventArgs_stub_vtbl,
    ICoreWebView2SharedWorkerCreatedEventArgs_iids,
};

/* ===== ICoreWebView2SharedWorkerCollectionView ===== */

static const IID *const ICoreWebView2SharedWorkerCollectionView_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2SharedWorkerCollectionView,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2SharedWorkerCollectionView_get_Count(ICoreWebView2SharedWorkerCollectionView *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SharedWorkerCollectionView_GetValueAtIndex(ICoreWebView2SharedWorkerCollectionView *iface, UINT32 index, ICoreWebView2SharedWorker **value)
{
    FIXME("iface %p, index %d, value %p stub!\n", iface, (int)index, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2SharedWorker);
    return S_OK;
}

static const ICoreWebView2SharedWorkerCollectionViewVtbl ICoreWebView2SharedWorkerCollectionView_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2SharedWorkerCollectionView_get_Count,
    ICoreWebView2SharedWorkerCollectionView_GetValueAtIndex,
};

const struct stub_family stub_family_ICoreWebView2SharedWorkerCollectionView =
{
    "ICoreWebView2SharedWorkerCollectionView",
    &ICoreWebView2SharedWorkerCollectionView_stub_vtbl,
    ICoreWebView2SharedWorkerCollectionView_iids,
};

/* ===== ICoreWebView2SharedWorkerManager ===== */

static const IID *const ICoreWebView2SharedWorkerManager_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2SharedWorkerManager,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2SharedWorkerManager_add_SharedWorkerCreated(ICoreWebView2SharedWorkerManager *iface, ICoreWebView2SharedWorkerCreatedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SharedWorkerManager_remove_SharedWorkerCreated(ICoreWebView2SharedWorkerManager *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2SharedWorkerManager_GetSharedWorkers(ICoreWebView2SharedWorkerManager *iface, ICoreWebView2GetSharedWorkersCompletedHandler *handler)
{
    FIXME("iface %p, handler %p stub!\n", iface, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2GetSharedWorkersCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static const ICoreWebView2SharedWorkerManagerVtbl ICoreWebView2SharedWorkerManager_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2SharedWorkerManager_add_SharedWorkerCreated,
    ICoreWebView2SharedWorkerManager_remove_SharedWorkerCreated,
    ICoreWebView2SharedWorkerManager_GetSharedWorkers,
};

const struct stub_family stub_family_ICoreWebView2SharedWorkerManager =
{
    "ICoreWebView2SharedWorkerManager",
    &ICoreWebView2SharedWorkerManager_stub_vtbl,
    ICoreWebView2SharedWorkerManager_iids,
};

/* ===== ICoreWebView2SourceChangedEventArgs ===== */

static const IID *const ICoreWebView2SourceChangedEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2SourceChangedEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2SourceChangedEventArgs_get_IsNewDocument(ICoreWebView2SourceChangedEventArgs *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2SourceChangedEventArgsVtbl ICoreWebView2SourceChangedEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2SourceChangedEventArgs_get_IsNewDocument,
};

const struct stub_family stub_family_ICoreWebView2SourceChangedEventArgs =
{
    "ICoreWebView2SourceChangedEventArgs",
    &ICoreWebView2SourceChangedEventArgs_stub_vtbl,
    ICoreWebView2SourceChangedEventArgs_iids,
};

/* ===== ICoreWebView2WebMessageReceivedEventArgs2 ===== */

static const IID *const ICoreWebView2WebMessageReceivedEventArgs2_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2WebMessageReceivedEventArgs,
    &IID_ICoreWebView2WebMessageReceivedEventArgs2,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebMessageReceivedEventArgs_get_Source(ICoreWebView2WebMessageReceivedEventArgs2 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebMessageReceivedEventArgs_get_WebMessageAsJson(ICoreWebView2WebMessageReceivedEventArgs2 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebMessageReceivedEventArgs_TryGetWebMessageAsString(ICoreWebView2WebMessageReceivedEventArgs2 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebMessageReceivedEventArgs2_get_AdditionalObjects(ICoreWebView2WebMessageReceivedEventArgs2 *iface, ICoreWebView2ObjectCollectionView **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2ObjectCollection);
    return S_OK;
}

static const ICoreWebView2WebMessageReceivedEventArgs2Vtbl ICoreWebView2WebMessageReceivedEventArgs2_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2WebMessageReceivedEventArgs_get_Source,
    ICoreWebView2WebMessageReceivedEventArgs_get_WebMessageAsJson,
    ICoreWebView2WebMessageReceivedEventArgs_TryGetWebMessageAsString,
    ICoreWebView2WebMessageReceivedEventArgs2_get_AdditionalObjects,
};

const struct stub_family stub_family_ICoreWebView2WebMessageReceivedEventArgs2 =
{
    "ICoreWebView2WebMessageReceivedEventArgs2",
    &ICoreWebView2WebMessageReceivedEventArgs2_stub_vtbl,
    ICoreWebView2WebMessageReceivedEventArgs2_iids,
};

/* ===== ICoreWebView2WebResourceRequestedEventArgs2 ===== */

static const IID *const ICoreWebView2WebResourceRequestedEventArgs2_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2WebResourceRequestedEventArgs,
    &IID_ICoreWebView2WebResourceRequestedEventArgs2,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceRequestedEventArgs_get_Request(ICoreWebView2WebResourceRequestedEventArgs2 *iface, ICoreWebView2WebResourceRequest **request)
{
    FIXME("iface %p, request %p stub!\n", iface, request);
    if (request) *request = stub_create(&stub_family_ICoreWebView2WebResourceRequest);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceRequestedEventArgs_get_Response(ICoreWebView2WebResourceRequestedEventArgs2 *iface, ICoreWebView2WebResourceResponse **response)
{
    FIXME("iface %p, response %p stub!\n", iface, response);
    if (response) *response = stub_create(&stub_family_ICoreWebView2WebResourceResponse);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceRequestedEventArgs_put_Response(ICoreWebView2WebResourceRequestedEventArgs2 *iface, ICoreWebView2WebResourceResponse *response)
{
    FIXME("iface %p, response %p stub!\n", iface, (void *)response);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceRequestedEventArgs_GetDeferral(ICoreWebView2WebResourceRequestedEventArgs2 *iface, ICoreWebView2Deferral **deferral)
{
    FIXME("iface %p, deferral %p stub!\n", iface, deferral);
    if (deferral) *deferral = stub_create(&stub_family_ICoreWebView2Deferral);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceRequestedEventArgs_get_ResourceContext(ICoreWebView2WebResourceRequestedEventArgs2 *iface, COREWEBVIEW2_WEB_RESOURCE_CONTEXT *context)
{
    FIXME("iface %p, context %p stub!\n", iface, context);
    if (context) memset(context, 0, sizeof(*context));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceRequestedEventArgs2_get_RequestedSourceKind(ICoreWebView2WebResourceRequestedEventArgs2 *iface, COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static const ICoreWebView2WebResourceRequestedEventArgs2Vtbl ICoreWebView2WebResourceRequestedEventArgs2_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2WebResourceRequestedEventArgs_get_Request,
    ICoreWebView2WebResourceRequestedEventArgs_get_Response,
    ICoreWebView2WebResourceRequestedEventArgs_put_Response,
    ICoreWebView2WebResourceRequestedEventArgs_GetDeferral,
    ICoreWebView2WebResourceRequestedEventArgs_get_ResourceContext,
    ICoreWebView2WebResourceRequestedEventArgs2_get_RequestedSourceKind,
};

const struct stub_family stub_family_ICoreWebView2WebResourceRequestedEventArgs2 =
{
    "ICoreWebView2WebResourceRequestedEventArgs2",
    &ICoreWebView2WebResourceRequestedEventArgs2_stub_vtbl,
    ICoreWebView2WebResourceRequestedEventArgs2_iids,
};

/* ===== ICoreWebView2WebResourceResponseReceivedEventArgs ===== */

static const IID *const ICoreWebView2WebResourceResponseReceivedEventArgs_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2WebResourceResponseReceivedEventArgs,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceResponseReceivedEventArgs_get_Request(ICoreWebView2WebResourceResponseReceivedEventArgs *iface, ICoreWebView2WebResourceRequest **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2WebResourceRequest);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2WebResourceResponseReceivedEventArgs_get_Response(ICoreWebView2WebResourceResponseReceivedEventArgs *iface, ICoreWebView2WebResourceResponseView **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2WebResourceResponseView);
    return S_OK;
}

static const ICoreWebView2WebResourceResponseReceivedEventArgsVtbl ICoreWebView2WebResourceResponseReceivedEventArgs_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2WebResourceResponseReceivedEventArgs_get_Request,
    ICoreWebView2WebResourceResponseReceivedEventArgs_get_Response,
};

const struct stub_family stub_family_ICoreWebView2WebResourceResponseReceivedEventArgs =
{
    "ICoreWebView2WebResourceResponseReceivedEventArgs",
    &ICoreWebView2WebResourceResponseReceivedEventArgs_stub_vtbl,
    ICoreWebView2WebResourceResponseReceivedEventArgs_iids,
};

/* ===== ICoreWebView2_29 ===== */

static const IID *const ICoreWebView2_29_iids[] =
{
    &IID_IUnknown,
    &IID_ICoreWebView2,
    &IID_ICoreWebView2_2,
    &IID_ICoreWebView2_3,
    &IID_ICoreWebView2_4,
    &IID_ICoreWebView2_5,
    &IID_ICoreWebView2_6,
    &IID_ICoreWebView2_7,
    &IID_ICoreWebView2_8,
    &IID_ICoreWebView2_9,
    &IID_ICoreWebView2_10,
    &IID_ICoreWebView2_11,
    &IID_ICoreWebView2_12,
    &IID_ICoreWebView2_13,
    &IID_ICoreWebView2_14,
    &IID_ICoreWebView2_15,
    &IID_ICoreWebView2_16,
    &IID_ICoreWebView2_17,
    &IID_ICoreWebView2_18,
    &IID_ICoreWebView2_19,
    &IID_ICoreWebView2_20,
    &IID_ICoreWebView2_21,
    &IID_ICoreWebView2_22,
    &IID_ICoreWebView2_23,
    &IID_ICoreWebView2_24,
    &IID_ICoreWebView2_25,
    &IID_ICoreWebView2_26,
    &IID_ICoreWebView2_27,
    &IID_ICoreWebView2_28,
    &IID_ICoreWebView2_29,
    NULL
};

static HRESULT STDMETHODCALLTYPE ICoreWebView2_get_Settings(ICoreWebView2_29 *iface, ICoreWebView2Settings **settings)
{
    FIXME("iface %p, settings %p stub!\n", iface, settings);
    if (settings) *settings = stub_create(&stub_family_ICoreWebView2Settings9);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_get_Source(ICoreWebView2_29 *iface, LPWSTR *uri)
{
    FIXME("iface %p, uri %p stub!\n", iface, uri);
    if (uri) *uri = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_Navigate(ICoreWebView2_29 *iface, LPCWSTR uri)
{
    FIXME("iface %p, uri %s stub!\n", iface, debugstr_w(uri));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_NavigateToString(ICoreWebView2_29 *iface, LPCWSTR htmlContent)
{
    FIXME("iface %p, htmlContent %s stub!\n", iface, debugstr_w(htmlContent));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_add_NavigationStarting(ICoreWebView2_29 *iface, ICoreWebView2NavigationStartingEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_remove_NavigationStarting(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_add_ContentLoading(ICoreWebView2_29 *iface, ICoreWebView2ContentLoadingEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_remove_ContentLoading(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_add_SourceChanged(ICoreWebView2_29 *iface, ICoreWebView2SourceChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_remove_SourceChanged(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_add_HistoryChanged(ICoreWebView2_29 *iface, ICoreWebView2HistoryChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_remove_HistoryChanged(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_add_NavigationCompleted(ICoreWebView2_29 *iface, ICoreWebView2NavigationCompletedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_remove_NavigationCompleted(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_add_FrameNavigationStarting(ICoreWebView2_29 *iface, ICoreWebView2NavigationStartingEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_remove_FrameNavigationStarting(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_add_FrameNavigationCompleted(ICoreWebView2_29 *iface, ICoreWebView2NavigationCompletedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_remove_FrameNavigationCompleted(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_add_ScriptDialogOpening(ICoreWebView2_29 *iface, ICoreWebView2ScriptDialogOpeningEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_remove_ScriptDialogOpening(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_add_PermissionRequested(ICoreWebView2_29 *iface, ICoreWebView2PermissionRequestedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_remove_PermissionRequested(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_add_ProcessFailed(ICoreWebView2_29 *iface, ICoreWebView2ProcessFailedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_remove_ProcessFailed(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_AddScriptToExecuteOnDocumentCreated(ICoreWebView2_29 *iface, LPCWSTR javaScript, ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler *handler)
{
    FIXME("iface %p, javaScript %s, handler %p stub!\n", iface, debugstr_w(javaScript), (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_RemoveScriptToExecuteOnDocumentCreated(ICoreWebView2_29 *iface, LPCWSTR id)
{
    FIXME("iface %p, id %s stub!\n", iface, debugstr_w(id));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_ExecuteScript(ICoreWebView2_29 *iface, LPCWSTR javaScript, ICoreWebView2ExecuteScriptCompletedHandler *handler)
{
    FIXME("iface %p, javaScript %s, handler %p stub!\n", iface, debugstr_w(javaScript), (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2ExecuteScriptCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_CapturePreview(ICoreWebView2_29 *iface, COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT imageFormat, IStream *imageStream, ICoreWebView2CapturePreviewCompletedHandler *handler)
{
    FIXME("iface %p, imageFormat %d, imageStream %p, handler %p stub!\n", iface, (int)imageFormat, (void *)imageStream, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2CapturePreviewCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_Reload(ICoreWebView2_29 *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_PostWebMessageAsJson(ICoreWebView2_29 *iface, LPCWSTR webMessageAsJson)
{
    FIXME("iface %p, webMessageAsJson %s stub!\n", iface, debugstr_w(webMessageAsJson));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_PostWebMessageAsString(ICoreWebView2_29 *iface, LPCWSTR webMessageAsString)
{
    FIXME("iface %p, webMessageAsString %s stub!\n", iface, debugstr_w(webMessageAsString));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_add_WebMessageReceived(ICoreWebView2_29 *iface, ICoreWebView2WebMessageReceivedEventHandler *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub!\n", iface, (void *)handler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_remove_WebMessageReceived(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_CallDevToolsProtocolMethod(ICoreWebView2_29 *iface, LPCWSTR methodName, LPCWSTR parametersAsJson, ICoreWebView2CallDevToolsProtocolMethodCompletedHandler *handler)
{
    FIXME("iface %p, methodName %s, parametersAsJson %s, handler %p stub!\n", iface, debugstr_w(methodName), debugstr_w(parametersAsJson), (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2CallDevToolsProtocolMethodCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_get_BrowserProcessId(ICoreWebView2_29 *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_get_CanGoBack(ICoreWebView2_29 *iface, BOOL *canGoBack)
{
    FIXME("iface %p, canGoBack %p stub!\n", iface, canGoBack);
    if (canGoBack) memset(canGoBack, 0, sizeof(*canGoBack));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_get_CanGoForward(ICoreWebView2_29 *iface, BOOL *canGoForward)
{
    FIXME("iface %p, canGoForward %p stub!\n", iface, canGoForward);
    if (canGoForward) memset(canGoForward, 0, sizeof(*canGoForward));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_GoBack(ICoreWebView2_29 *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_GoForward(ICoreWebView2_29 *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_GetDevToolsProtocolEventReceiver(ICoreWebView2_29 *iface, LPCWSTR eventName, ICoreWebView2DevToolsProtocolEventReceiver **receiver)
{
    FIXME("iface %p, eventName %s, receiver %p stub!\n", iface, debugstr_w(eventName), receiver);
    if (receiver) *receiver = stub_create(&stub_family_ICoreWebView2DevToolsProtocolEventReceiver);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_Stop(ICoreWebView2_29 *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_add_NewWindowRequested(ICoreWebView2_29 *iface, ICoreWebView2NewWindowRequestedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_remove_NewWindowRequested(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_add_DocumentTitleChanged(ICoreWebView2_29 *iface, ICoreWebView2DocumentTitleChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_remove_DocumentTitleChanged(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_get_DocumentTitle(ICoreWebView2_29 *iface, LPWSTR *title)
{
    FIXME("iface %p, title %p stub!\n", iface, title);
    if (title) *title = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_AddHostObjectToScript(ICoreWebView2_29 *iface, LPCWSTR name, VARIANT *object)
{
    FIXME("iface %p, name %s, object %p stub!\n", iface, debugstr_w(name), (void *)object);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_RemoveHostObjectFromScript(ICoreWebView2_29 *iface, LPCWSTR name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_w(name));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_OpenDevToolsWindow(ICoreWebView2_29 *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_add_ContainsFullScreenElementChanged(ICoreWebView2_29 *iface, ICoreWebView2ContainsFullScreenElementChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_remove_ContainsFullScreenElementChanged(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_get_ContainsFullScreenElement(ICoreWebView2_29 *iface, BOOL *containsFullScreenElement)
{
    FIXME("iface %p, containsFullScreenElement %p stub!\n", iface, containsFullScreenElement);
    if (containsFullScreenElement) memset(containsFullScreenElement, 0, sizeof(*containsFullScreenElement));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_add_WebResourceRequested(ICoreWebView2_29 *iface, ICoreWebView2WebResourceRequestedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_remove_WebResourceRequested(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_AddWebResourceRequestedFilter(ICoreWebView2_29 *iface, const LPCWSTR uri, const COREWEBVIEW2_WEB_RESOURCE_CONTEXT resourceContext)
{
    FIXME("iface %p, uri %s, resourceContext %d stub!\n", iface, debugstr_w(uri), (int)resourceContext);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_RemoveWebResourceRequestedFilter(ICoreWebView2_29 *iface, const LPCWSTR uri, const COREWEBVIEW2_WEB_RESOURCE_CONTEXT resourceContext)
{
    FIXME("iface %p, uri %s, resourceContext %d stub!\n", iface, debugstr_w(uri), (int)resourceContext);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_add_WindowCloseRequested(ICoreWebView2_29 *iface, ICoreWebView2WindowCloseRequestedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_remove_WindowCloseRequested(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_2_add_WebResourceResponseReceived(ICoreWebView2_29 *iface, ICoreWebView2WebResourceResponseReceivedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_2_remove_WebResourceResponseReceived(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_2_NavigateWithWebResourceRequest(ICoreWebView2_29 *iface, ICoreWebView2WebResourceRequest *request)
{
    FIXME("iface %p, request %p stub!\n", iface, (void *)request);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_2_add_DOMContentLoaded(ICoreWebView2_29 *iface, ICoreWebView2DOMContentLoadedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_2_remove_DOMContentLoaded(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_2_get_CookieManager(ICoreWebView2_29 *iface, ICoreWebView2CookieManager **cookieManager)
{
    FIXME("iface %p, cookieManager %p stub!\n", iface, cookieManager);
    if (cookieManager) *cookieManager = stub_create(&stub_family_ICoreWebView2CookieManager);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_2_get_Environment(ICoreWebView2_29 *iface, ICoreWebView2Environment **environment)
{
    FIXME("iface %p, environment %p stub!\n", iface, environment);
    if (environment) *environment = stub_create(&stub_family_ICoreWebView2Environment15);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_3_TrySuspend(ICoreWebView2_29 *iface, ICoreWebView2TrySuspendCompletedHandler *handler)
{
    FIXME("iface %p, handler %p stub!\n", iface, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2TrySuspendCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_3_Resume(ICoreWebView2_29 *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_3_get_IsSuspended(ICoreWebView2_29 *iface, BOOL *isSuspended)
{
    FIXME("iface %p, isSuspended %p stub!\n", iface, isSuspended);
    if (isSuspended) memset(isSuspended, 0, sizeof(*isSuspended));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_3_SetVirtualHostNameToFolderMapping(ICoreWebView2_29 *iface, LPCWSTR hostName, LPCWSTR folderPath, COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND accessKind)
{
    FIXME("iface %p, hostName %s, folderPath %s, accessKind %d stub!\n", iface, debugstr_w(hostName), debugstr_w(folderPath), (int)accessKind);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_3_ClearVirtualHostNameToFolderMapping(ICoreWebView2_29 *iface, LPCWSTR hostName)
{
    FIXME("iface %p, hostName %s stub!\n", iface, debugstr_w(hostName));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_4_add_FrameCreated(ICoreWebView2_29 *iface, ICoreWebView2FrameCreatedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_4_remove_FrameCreated(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_4_add_DownloadStarting(ICoreWebView2_29 *iface, ICoreWebView2DownloadStartingEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_4_remove_DownloadStarting(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_5_add_ClientCertificateRequested(ICoreWebView2_29 *iface, ICoreWebView2ClientCertificateRequestedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_5_remove_ClientCertificateRequested(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_6_OpenTaskManagerWindow(ICoreWebView2_29 *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_7_PrintToPdf(ICoreWebView2_29 *iface, LPCWSTR ResultFilePath, ICoreWebView2PrintSettings *printSettings, ICoreWebView2PrintToPdfCompletedHandler *handler)
{
    FIXME("iface %p, ResultFilePath %s, printSettings %p, handler %p stub!\n", iface, debugstr_w(ResultFilePath), (void *)printSettings, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2PrintToPdfCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_8_add_IsMutedChanged(ICoreWebView2_29 *iface, ICoreWebView2IsMutedChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_8_remove_IsMutedChanged(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_8_get_IsMuted(ICoreWebView2_29 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_8_put_IsMuted(ICoreWebView2_29 *iface, BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_8_add_IsDocumentPlayingAudioChanged(ICoreWebView2_29 *iface, ICoreWebView2IsDocumentPlayingAudioChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_8_remove_IsDocumentPlayingAudioChanged(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_8_get_IsDocumentPlayingAudio(ICoreWebView2_29 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_9_add_IsDefaultDownloadDialogOpenChanged(ICoreWebView2_29 *iface, ICoreWebView2IsDefaultDownloadDialogOpenChangedEventHandler *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub!\n", iface, (void *)handler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_9_remove_IsDefaultDownloadDialogOpenChanged(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_9_get_IsDefaultDownloadDialogOpen(ICoreWebView2_29 *iface, BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_9_OpenDefaultDownloadDialog(ICoreWebView2_29 *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_9_CloseDefaultDownloadDialog(ICoreWebView2_29 *iface)
{
    FIXME("iface %p stub!\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_9_get_DefaultDownloadDialogCornerAlignment(ICoreWebView2_29 *iface, COREWEBVIEW2_DEFAULT_DOWNLOAD_DIALOG_CORNER_ALIGNMENT *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_9_put_DefaultDownloadDialogCornerAlignment(ICoreWebView2_29 *iface, COREWEBVIEW2_DEFAULT_DOWNLOAD_DIALOG_CORNER_ALIGNMENT value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_9_get_DefaultDownloadDialogMargin(ICoreWebView2_29 *iface, POINT *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_9_put_DefaultDownloadDialogMargin(ICoreWebView2_29 *iface, POINT value)
{
    FIXME("iface %p, value %s stub!\n", iface, wine_dbgstr_point(&value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_10_add_BasicAuthenticationRequested(ICoreWebView2_29 *iface, ICoreWebView2BasicAuthenticationRequestedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_10_remove_BasicAuthenticationRequested(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_11_CallDevToolsProtocolMethodForSession(ICoreWebView2_29 *iface, LPCWSTR sessionId, LPCWSTR methodName, LPCWSTR parametersAsJson, ICoreWebView2CallDevToolsProtocolMethodCompletedHandler *handler)
{
    FIXME("iface %p, sessionId %s, methodName %s, parametersAsJson %s, handler %p stub!\n", iface, debugstr_w(sessionId), debugstr_w(methodName), debugstr_w(parametersAsJson), (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2CallDevToolsProtocolMethodCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_11_add_ContextMenuRequested(ICoreWebView2_29 *iface, ICoreWebView2ContextMenuRequestedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_11_remove_ContextMenuRequested(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_12_add_StatusBarTextChanged(ICoreWebView2_29 *iface, ICoreWebView2StatusBarTextChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_12_remove_StatusBarTextChanged(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_12_get_StatusBarText(ICoreWebView2_29 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_13_get_Profile(ICoreWebView2_29 *iface, ICoreWebView2Profile **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2Profile9);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_14_add_ServerCertificateErrorDetected(ICoreWebView2_29 *iface, ICoreWebView2ServerCertificateErrorDetectedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_14_remove_ServerCertificateErrorDetected(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_14_ClearServerCertificateErrorActions(ICoreWebView2_29 *iface, ICoreWebView2ClearServerCertificateErrorActionsCompletedHandler *handler)
{
    FIXME("iface %p, handler %p stub!\n", iface, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2ClearServerCertificateErrorActionsCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_15_add_FaviconChanged(ICoreWebView2_29 *iface, ICoreWebView2FaviconChangedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_15_remove_FaviconChanged(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_15_get_FaviconUri(ICoreWebView2_29 *iface, LPWSTR *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_strdup(L"");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_15_GetFavicon(ICoreWebView2_29 *iface, COREWEBVIEW2_FAVICON_IMAGE_FORMAT format, ICoreWebView2GetFaviconCompletedHandler *completedHandler)
{
    FIXME("iface %p, format %d, completedHandler %p stub!\n", iface, (int)format, (void *)completedHandler);
    if (completedHandler) stub_invoke_async(invoke_ICoreWebView2GetFaviconCompletedHandler, (IUnknown *)completedHandler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_16_Print(ICoreWebView2_29 *iface, ICoreWebView2PrintSettings *printSettings, ICoreWebView2PrintCompletedHandler *handler)
{
    FIXME("iface %p, printSettings %p, handler %p stub!\n", iface, (void *)printSettings, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2PrintCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_16_ShowPrintUI(ICoreWebView2_29 *iface, COREWEBVIEW2_PRINT_DIALOG_KIND printDialogKind)
{
    FIXME("iface %p, printDialogKind %d stub!\n", iface, (int)printDialogKind);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_16_PrintToPdfStream(ICoreWebView2_29 *iface, ICoreWebView2PrintSettings *printSettings, ICoreWebView2PrintToPdfStreamCompletedHandler *handler)
{
    FIXME("iface %p, printSettings %p, handler %p stub!\n", iface, (void *)printSettings, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2PrintToPdfStreamCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_17_PostSharedBufferToScript(ICoreWebView2_29 *iface, ICoreWebView2SharedBuffer *sharedBuffer, COREWEBVIEW2_SHARED_BUFFER_ACCESS access, LPCWSTR additionalDataAsJson)
{
    FIXME("iface %p, sharedBuffer %p, access %d, additionalDataAsJson %s stub!\n", iface, (void *)sharedBuffer, (int)access, debugstr_w(additionalDataAsJson));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_18_add_LaunchingExternalUriScheme(ICoreWebView2_29 *iface, ICoreWebView2LaunchingExternalUriSchemeEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_18_remove_LaunchingExternalUriScheme(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_19_get_MemoryUsageTargetLevel(ICoreWebView2_29 *iface, COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_19_put_MemoryUsageTargetLevel(ICoreWebView2_29 *iface, COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL value)
{
    FIXME("iface %p, value %d stub!\n", iface, (int)value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_20_get_FrameId(ICoreWebView2_29 *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) memset(value, 0, sizeof(*value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_21_ExecuteScriptWithResult(ICoreWebView2_29 *iface, LPCWSTR javaScript, ICoreWebView2ExecuteScriptWithResultCompletedHandler *handler)
{
    FIXME("iface %p, javaScript %s, handler %p stub!\n", iface, debugstr_w(javaScript), (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2ExecuteScriptWithResultCompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_22_AddWebResourceRequestedFilterWithRequestSourceKinds(ICoreWebView2_29 *iface, LPCWSTR uri, COREWEBVIEW2_WEB_RESOURCE_CONTEXT ResourceContext, COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS requestSourceKinds)
{
    FIXME("iface %p, uri %s, ResourceContext %d, requestSourceKinds %d stub!\n", iface, debugstr_w(uri), (int)ResourceContext, (int)requestSourceKinds);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_22_RemoveWebResourceRequestedFilterWithRequestSourceKinds(ICoreWebView2_29 *iface, LPCWSTR uri, COREWEBVIEW2_WEB_RESOURCE_CONTEXT ResourceContext, COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS requestSourceKinds)
{
    FIXME("iface %p, uri %s, ResourceContext %d, requestSourceKinds %d stub!\n", iface, debugstr_w(uri), (int)ResourceContext, (int)requestSourceKinds);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_23_PostWebMessageAsJsonWithAdditionalObjects(ICoreWebView2_29 *iface, LPCWSTR webMessageAsJson, ICoreWebView2ObjectCollectionView *additionalObjects)
{
    FIXME("iface %p, webMessageAsJson %s, additionalObjects %p stub!\n", iface, debugstr_w(webMessageAsJson), (void *)additionalObjects);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_24_add_NotificationReceived(ICoreWebView2_29 *iface, ICoreWebView2NotificationReceivedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_24_remove_NotificationReceived(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_25_add_SaveAsUIShowing(ICoreWebView2_29 *iface, ICoreWebView2SaveAsUIShowingEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_25_remove_SaveAsUIShowing(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_25_ShowSaveAsUI(ICoreWebView2_29 *iface, ICoreWebView2ShowSaveAsUICompletedHandler *handler)
{
    FIXME("iface %p, handler %p stub!\n", iface, (void *)handler);
    if (handler) stub_invoke_async(invoke_ICoreWebView2ShowSaveAsUICompletedHandler, (IUnknown *)handler);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_26_add_SaveFileSecurityCheckStarting(ICoreWebView2_29 *iface, ICoreWebView2SaveFileSecurityCheckStartingEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_26_remove_SaveFileSecurityCheckStarting(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_27_add_ScreenCaptureStarting(ICoreWebView2_29 *iface, ICoreWebView2ScreenCaptureStartingEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_27_remove_ScreenCaptureStarting(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_28_get_Find(ICoreWebView2_29 *iface, ICoreWebView2Find **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    if (value) *value = stub_create(&stub_family_ICoreWebView2Find);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_29_add_DedicatedWorkerCreated(ICoreWebView2_29 *iface, ICoreWebView2DedicatedWorkerCreatedEventHandler *eventHandler, EventRegistrationToken *token)
{
    FIXME("iface %p, eventHandler %p, token %p stub!\n", iface, (void *)eventHandler, token);
    if (token) token->value = stub_next_token();
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE ICoreWebView2_29_remove_DedicatedWorkerCreated(ICoreWebView2_29 *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %s stub!\n", iface, wine_dbgstr_longlong(token.value));
    return S_OK;
}

static const ICoreWebView2_29Vtbl ICoreWebView2_29_stub_vtbl =
{
    (void *)stub_QueryInterface,
    (void *)stub_AddRef,
    (void *)stub_Release,
    ICoreWebView2_get_Settings,
    ICoreWebView2_get_Source,
    ICoreWebView2_Navigate,
    ICoreWebView2_NavigateToString,
    ICoreWebView2_add_NavigationStarting,
    ICoreWebView2_remove_NavigationStarting,
    ICoreWebView2_add_ContentLoading,
    ICoreWebView2_remove_ContentLoading,
    ICoreWebView2_add_SourceChanged,
    ICoreWebView2_remove_SourceChanged,
    ICoreWebView2_add_HistoryChanged,
    ICoreWebView2_remove_HistoryChanged,
    ICoreWebView2_add_NavigationCompleted,
    ICoreWebView2_remove_NavigationCompleted,
    ICoreWebView2_add_FrameNavigationStarting,
    ICoreWebView2_remove_FrameNavigationStarting,
    ICoreWebView2_add_FrameNavigationCompleted,
    ICoreWebView2_remove_FrameNavigationCompleted,
    ICoreWebView2_add_ScriptDialogOpening,
    ICoreWebView2_remove_ScriptDialogOpening,
    ICoreWebView2_add_PermissionRequested,
    ICoreWebView2_remove_PermissionRequested,
    ICoreWebView2_add_ProcessFailed,
    ICoreWebView2_remove_ProcessFailed,
    ICoreWebView2_AddScriptToExecuteOnDocumentCreated,
    ICoreWebView2_RemoveScriptToExecuteOnDocumentCreated,
    ICoreWebView2_ExecuteScript,
    ICoreWebView2_CapturePreview,
    ICoreWebView2_Reload,
    ICoreWebView2_PostWebMessageAsJson,
    ICoreWebView2_PostWebMessageAsString,
    ICoreWebView2_add_WebMessageReceived,
    ICoreWebView2_remove_WebMessageReceived,
    ICoreWebView2_CallDevToolsProtocolMethod,
    ICoreWebView2_get_BrowserProcessId,
    ICoreWebView2_get_CanGoBack,
    ICoreWebView2_get_CanGoForward,
    ICoreWebView2_GoBack,
    ICoreWebView2_GoForward,
    ICoreWebView2_GetDevToolsProtocolEventReceiver,
    ICoreWebView2_Stop,
    ICoreWebView2_add_NewWindowRequested,
    ICoreWebView2_remove_NewWindowRequested,
    ICoreWebView2_add_DocumentTitleChanged,
    ICoreWebView2_remove_DocumentTitleChanged,
    ICoreWebView2_get_DocumentTitle,
    ICoreWebView2_AddHostObjectToScript,
    ICoreWebView2_RemoveHostObjectFromScript,
    ICoreWebView2_OpenDevToolsWindow,
    ICoreWebView2_add_ContainsFullScreenElementChanged,
    ICoreWebView2_remove_ContainsFullScreenElementChanged,
    ICoreWebView2_get_ContainsFullScreenElement,
    ICoreWebView2_add_WebResourceRequested,
    ICoreWebView2_remove_WebResourceRequested,
    ICoreWebView2_AddWebResourceRequestedFilter,
    ICoreWebView2_RemoveWebResourceRequestedFilter,
    ICoreWebView2_add_WindowCloseRequested,
    ICoreWebView2_remove_WindowCloseRequested,
    ICoreWebView2_2_add_WebResourceResponseReceived,
    ICoreWebView2_2_remove_WebResourceResponseReceived,
    ICoreWebView2_2_NavigateWithWebResourceRequest,
    ICoreWebView2_2_add_DOMContentLoaded,
    ICoreWebView2_2_remove_DOMContentLoaded,
    ICoreWebView2_2_get_CookieManager,
    ICoreWebView2_2_get_Environment,
    ICoreWebView2_3_TrySuspend,
    ICoreWebView2_3_Resume,
    ICoreWebView2_3_get_IsSuspended,
    ICoreWebView2_3_SetVirtualHostNameToFolderMapping,
    ICoreWebView2_3_ClearVirtualHostNameToFolderMapping,
    ICoreWebView2_4_add_FrameCreated,
    ICoreWebView2_4_remove_FrameCreated,
    ICoreWebView2_4_add_DownloadStarting,
    ICoreWebView2_4_remove_DownloadStarting,
    ICoreWebView2_5_add_ClientCertificateRequested,
    ICoreWebView2_5_remove_ClientCertificateRequested,
    ICoreWebView2_6_OpenTaskManagerWindow,
    ICoreWebView2_7_PrintToPdf,
    ICoreWebView2_8_add_IsMutedChanged,
    ICoreWebView2_8_remove_IsMutedChanged,
    ICoreWebView2_8_get_IsMuted,
    ICoreWebView2_8_put_IsMuted,
    ICoreWebView2_8_add_IsDocumentPlayingAudioChanged,
    ICoreWebView2_8_remove_IsDocumentPlayingAudioChanged,
    ICoreWebView2_8_get_IsDocumentPlayingAudio,
    ICoreWebView2_9_add_IsDefaultDownloadDialogOpenChanged,
    ICoreWebView2_9_remove_IsDefaultDownloadDialogOpenChanged,
    ICoreWebView2_9_get_IsDefaultDownloadDialogOpen,
    ICoreWebView2_9_OpenDefaultDownloadDialog,
    ICoreWebView2_9_CloseDefaultDownloadDialog,
    ICoreWebView2_9_get_DefaultDownloadDialogCornerAlignment,
    ICoreWebView2_9_put_DefaultDownloadDialogCornerAlignment,
    ICoreWebView2_9_get_DefaultDownloadDialogMargin,
    ICoreWebView2_9_put_DefaultDownloadDialogMargin,
    ICoreWebView2_10_add_BasicAuthenticationRequested,
    ICoreWebView2_10_remove_BasicAuthenticationRequested,
    ICoreWebView2_11_CallDevToolsProtocolMethodForSession,
    ICoreWebView2_11_add_ContextMenuRequested,
    ICoreWebView2_11_remove_ContextMenuRequested,
    ICoreWebView2_12_add_StatusBarTextChanged,
    ICoreWebView2_12_remove_StatusBarTextChanged,
    ICoreWebView2_12_get_StatusBarText,
    ICoreWebView2_13_get_Profile,
    ICoreWebView2_14_add_ServerCertificateErrorDetected,
    ICoreWebView2_14_remove_ServerCertificateErrorDetected,
    ICoreWebView2_14_ClearServerCertificateErrorActions,
    ICoreWebView2_15_add_FaviconChanged,
    ICoreWebView2_15_remove_FaviconChanged,
    ICoreWebView2_15_get_FaviconUri,
    ICoreWebView2_15_GetFavicon,
    ICoreWebView2_16_Print,
    ICoreWebView2_16_ShowPrintUI,
    ICoreWebView2_16_PrintToPdfStream,
    ICoreWebView2_17_PostSharedBufferToScript,
    ICoreWebView2_18_add_LaunchingExternalUriScheme,
    ICoreWebView2_18_remove_LaunchingExternalUriScheme,
    ICoreWebView2_19_get_MemoryUsageTargetLevel,
    ICoreWebView2_19_put_MemoryUsageTargetLevel,
    ICoreWebView2_20_get_FrameId,
    ICoreWebView2_21_ExecuteScriptWithResult,
    ICoreWebView2_22_AddWebResourceRequestedFilterWithRequestSourceKinds,
    ICoreWebView2_22_RemoveWebResourceRequestedFilterWithRequestSourceKinds,
    ICoreWebView2_23_PostWebMessageAsJsonWithAdditionalObjects,
    ICoreWebView2_24_add_NotificationReceived,
    ICoreWebView2_24_remove_NotificationReceived,
    ICoreWebView2_25_add_SaveAsUIShowing,
    ICoreWebView2_25_remove_SaveAsUIShowing,
    ICoreWebView2_25_ShowSaveAsUI,
    ICoreWebView2_26_add_SaveFileSecurityCheckStarting,
    ICoreWebView2_26_remove_SaveFileSecurityCheckStarting,
    ICoreWebView2_27_add_ScreenCaptureStarting,
    ICoreWebView2_27_remove_ScreenCaptureStarting,
    ICoreWebView2_28_get_Find,
    ICoreWebView2_29_add_DedicatedWorkerCreated,
    ICoreWebView2_29_remove_DedicatedWorkerCreated,
};

const struct stub_family stub_family_ICoreWebView2_29 =
{
    "ICoreWebView2_29",
    &ICoreWebView2_29_stub_vtbl,
    ICoreWebView2_29_iids,
};
