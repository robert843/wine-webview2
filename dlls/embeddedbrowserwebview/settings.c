/*
 * ICoreWebView2Settings
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
#include "protocol.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(webview2);

/* Defaults documented for WebView2. Flags forwarded to the helper are WV2_SETTING_*; the others
 * are only stored (no equivalent behaviour yet). */
struct settings
{
    UINT32 view;
    UINT32 flags;           /* WV2_SETTING_* */
    BOOL status_bar;
    BOOL host_objects;
    BOOL builtin_error_page;
    BOOL password_autosave;
    BOOL general_autofill;
    BOOL pinch_zoom;
    BOOL swipe_navigation;
    BOOL reputation_checking;
    BOOL non_client_region;
    COREWEBVIEW2_PDF_TOOLBAR_ITEMS hidden_pdf_items;
    WCHAR *user_agent;      /* NULL = default */
};

static ICoreWebView2Settings9Vtbl settings_vtbl;
static struct stub_family settings_family;

static struct settings *impl_from_iface(ICoreWebView2Settings9 *iface)
{
    return ((struct stub_object *)iface)->data;
}

static void settings_destroy(struct stub_object *obj)
{
    struct settings *settings = obj->data;

    free(settings->user_agent);
    free(settings);
}

static HRESULT get_flag(ICoreWebView2Settings9 *iface, UINT32 flag, BOOL *value)
{
    if (!value) return E_POINTER;
    *value = !!(impl_from_iface(iface)->flags & flag);
    return S_OK;
}

static HRESULT put_flag(ICoreWebView2Settings9 *iface, UINT32 flag, BOOL value)
{
    struct settings *settings = impl_from_iface(iface);
    UINT32 flags = value ? settings->flags | flag : settings->flags & ~flag;
    struct wv2_flag msg = {flags, 0};

    if (flags == settings->flags) return S_OK;
    settings->flags = flags;
    webview2_send(settings->view, WV2_CMD_SET_SETTINGS, &msg, sizeof(msg));
    return S_OK;
}

static HRESULT get_bool(BOOL field, BOOL *value)
{
    if (!value) return E_POINTER;
    *value = field;
    return S_OK;
}

#define FLAG_PROPERTY(iface_name, name, flag) \
static HRESULT STDMETHODCALLTYPE settings_get_##name(ICoreWebView2Settings9 *iface, BOOL *value) \
{ \
    TRACE("iface %p, value %p.\n", iface, value); \
    return get_flag(iface, flag, value); \
} \
static HRESULT STDMETHODCALLTYPE settings_put_##name(ICoreWebView2Settings9 *iface, BOOL value) \
{ \
    TRACE("iface %p, value %d.\n", iface, value); \
    return put_flag(iface, flag, value); \
}

#define STORED_PROPERTY(name, field) \
static HRESULT STDMETHODCALLTYPE settings_get_##name(ICoreWebView2Settings9 *iface, BOOL *value) \
{ \
    TRACE("iface %p, value %p.\n", iface, value); \
    return get_bool(impl_from_iface(iface)->field, value); \
} \
static HRESULT STDMETHODCALLTYPE settings_put_##name(ICoreWebView2Settings9 *iface, BOOL value) \
{ \
    TRACE("iface %p, value %d: stored only.\n", iface, value); \
    impl_from_iface(iface)->field = !!value; \
    return S_OK; \
}

FLAG_PROPERTY(Settings, IsScriptEnabled, WV2_SETTING_SCRIPT)
FLAG_PROPERTY(Settings, IsWebMessageEnabled, WV2_SETTING_WEB_MESSAGE)
FLAG_PROPERTY(Settings, AreDefaultScriptDialogsEnabled, WV2_SETTING_SCRIPT_DIALOGS)
FLAG_PROPERTY(Settings, AreDefaultContextMenusEnabled, WV2_SETTING_CONTEXT_MENUS)
FLAG_PROPERTY(Settings, AreDevToolsEnabled, WV2_SETTING_DEVTOOLS)
FLAG_PROPERTY(Settings, IsZoomControlEnabled, WV2_SETTING_ZOOM)
FLAG_PROPERTY(Settings3, AreBrowserAcceleratorKeysEnabled, WV2_SETTING_ACCELERATORS)
STORED_PROPERTY(IsStatusBarEnabled, status_bar)
STORED_PROPERTY(AreHostObjectsAllowed, host_objects)
STORED_PROPERTY(IsBuiltInErrorPageEnabled, builtin_error_page)
STORED_PROPERTY(IsPasswordAutosaveEnabled, password_autosave)
STORED_PROPERTY(IsGeneralAutofillEnabled, general_autofill)
STORED_PROPERTY(IsPinchZoomEnabled, pinch_zoom)
STORED_PROPERTY(IsSwipeNavigationEnabled, swipe_navigation)
STORED_PROPERTY(IsReputationCheckingRequired, reputation_checking)
STORED_PROPERTY(IsNonClientRegionSupportEnabled, non_client_region)

static HRESULT STDMETHODCALLTYPE settings_get_UserAgent(ICoreWebView2Settings9 *iface, LPWSTR *ua)
{
    static const WCHAR default_ua[] = L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
            L"(KHTML, like Gecko) Chrome/" WEBVIEW2_VERSION_MAJOR_WSTR L".0.0.0 Safari/537.36 Edg/"
            WEBVIEW2_VERSION_MAJOR_WSTR L".0.0.0";
    struct settings *settings = impl_from_iface(iface);
    const WCHAR *str = settings->user_agent ? settings->user_agent : default_ua;
    size_t size = (wcslen(str) + 1) * sizeof(WCHAR);

    TRACE("iface %p, ua %p.\n", iface, ua);
    if (!ua) return E_POINTER;
    if (!(*ua = CoTaskMemAlloc(size))) return E_OUTOFMEMORY;
    memcpy(*ua, str, size);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE settings_put_UserAgent(ICoreWebView2Settings9 *iface, LPCWSTR ua)
{
    struct settings *settings = impl_from_iface(iface);
    WCHAR *copy = NULL;
    char *utf8;
    int len;

    TRACE("iface %p, ua %s.\n", iface, debugstr_w(ua));
    if (ua && *ua && !(copy = wcsdup(ua))) return E_OUTOFMEMORY;
    free(settings->user_agent);
    settings->user_agent = copy;

    len = WideCharToMultiByte(CP_UTF8, 0, copy ? copy : L"", -1, NULL, 0, NULL, NULL);
    if (!(utf8 = malloc(len))) return E_OUTOFMEMORY;
    WideCharToMultiByte(CP_UTF8, 0, copy ? copy : L"", -1, utf8, len, NULL, NULL);
    webview2_send(settings->view, WV2_CMD_SET_USER_AGENT, utf8, len - 1);
    free(utf8);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE settings_get_HiddenPdfToolbarItems(ICoreWebView2Settings9 *iface,
        COREWEBVIEW2_PDF_TOOLBAR_ITEMS *items)
{
    TRACE("iface %p, items %p.\n", iface, items);
    if (!items) return E_POINTER;
    *items = impl_from_iface(iface)->hidden_pdf_items;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE settings_put_HiddenPdfToolbarItems(ICoreWebView2Settings9 *iface,
        COREWEBVIEW2_PDF_TOOLBAR_ITEMS items)
{
    TRACE("iface %p, items %#x: stored only.\n", iface, items);
    impl_from_iface(iface)->hidden_pdf_items = items;
    return S_OK;
}

ICoreWebView2Settings *settings_create(UINT32 view)
{
    struct settings *settings;
    struct stub_object *obj;

    if (!(settings = calloc(1, sizeof(*settings)))) return NULL;
    if (!(obj = stub_create(&settings_family)))
    {
        free(settings);
        return NULL;
    }
    settings->view = view;
    settings->flags = WV2_SETTINGS_DEFAULT;
    settings->status_bar = TRUE;
    settings->host_objects = TRUE;
    settings->builtin_error_page = TRUE;
    settings->general_autofill = TRUE;
    settings->pinch_zoom = TRUE;
    settings->swipe_navigation = TRUE;
    settings->reputation_checking = TRUE;
    obj->data = settings;
    obj->destroy = settings_destroy;
    return (ICoreWebView2Settings *)obj;
}

/* Settings the WebView2 API cannot express; kept for the helper's view of them. */
UINT32 settings_get_flags(ICoreWebView2Settings *iface)
{
    return impl_from_iface((ICoreWebView2Settings9 *)iface)->flags;
}

void settings_init(void)
{
    settings_vtbl = *(const ICoreWebView2Settings9Vtbl *)stub_family_ICoreWebView2Settings9.vtbl;
#define SET(name) settings_vtbl.get_##name = settings_get_##name; settings_vtbl.put_##name = settings_put_##name
    SET(IsScriptEnabled);
    SET(IsWebMessageEnabled);
    SET(AreDefaultScriptDialogsEnabled);
    SET(IsStatusBarEnabled);
    SET(AreDevToolsEnabled);
    SET(AreDefaultContextMenusEnabled);
    SET(AreHostObjectsAllowed);
    SET(IsZoomControlEnabled);
    SET(IsBuiltInErrorPageEnabled);
    SET(UserAgent);
    SET(AreBrowserAcceleratorKeysEnabled);
    SET(IsPasswordAutosaveEnabled);
    SET(IsGeneralAutofillEnabled);
    SET(IsPinchZoomEnabled);
    SET(IsSwipeNavigationEnabled);
    SET(HiddenPdfToolbarItems);
    SET(IsReputationCheckingRequired);
    SET(IsNonClientRegionSupportEnabled);
#undef SET
    settings_family = stub_family_ICoreWebView2Settings9;
    settings_family.vtbl = &settings_vtbl;
}
