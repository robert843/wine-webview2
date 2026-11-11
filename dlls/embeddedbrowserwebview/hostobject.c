/*
 * Host objects (AddHostObjectToScript): IDispatch objects of the application called from
 * JavaScript through chrome.webview.hostObjects
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

/*
 * The renderer turns every operation on a hostObjects proxy into a WV2_EVT_HOST_OBJECT_CALL:
 * a root (named host object or an object id handed out earlier), a path of member names and
 * the operation on the last one. It runs here on the application's UI thread, like WebView2's
 * calls into host objects, and the result goes back as WV2_CMD_HOST_OBJECT_REPLY. IDispatch
 * results are kept in a table and sent as object ids, which the script wraps in new proxies.
 */

#include <stdarg.h>
#include <stdlib.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "oleauto.h"
#include "webview2.h"
#include "webview2_private.h"
#include "protocol.h"
#include "proxy.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(webview2);

struct named_object
{
    WCHAR *name;
    IDispatch *dispatch;
};

struct host_objects
{
    struct named_object *named;
    UINT32 named_count;
    IDispatch **objects;    /* object id = index + 1; NULL = released */
    UINT32 object_count;
};

static IDispatch *dispatch_from_variant(VARIANT *v)
{
    IDispatch *dispatch = NULL;

    if (V_VT(v) == VT_DISPATCH && V_DISPATCH(v)) IDispatch_AddRef(dispatch = V_DISPATCH(v));
    else if (V_VT(v) == VT_UNKNOWN && V_UNKNOWN(v))
        IUnknown_QueryInterface(V_UNKNOWN(v), &IID_IDispatch, (void **)&dispatch);
    return dispatch;
}

HRESULT host_objects_add(struct host_objects **objects, const WCHAR *name, VARIANT *object)
{
    struct host_objects *table = *objects;
    struct named_object *named;
    IDispatch *dispatch;
    UINT32 i;

    if (!name || !object) return E_INVALIDARG;
    /* WebView2 accepts IDispatch (or IUnknown with IDispatch), VT_DISPATCH arrays are not supported yet. */
    if (!(dispatch = dispatch_from_variant(object)))
    {
        FIXME("unsupported host object type %d\n", V_VT(object));
        return E_INVALIDARG;
    }
    if (!table && !(table = *objects = calloc(1, sizeof(*table))))
    {
        IDispatch_Release(dispatch);
        return E_OUTOFMEMORY;
    }
    for (i = 0; i < table->named_count; i++)
    {
        if (wcscmp(table->named[i].name, name)) continue;
        IDispatch_Release(table->named[i].dispatch);
        table->named[i].dispatch = dispatch;
        return S_OK;
    }
    if (!(named = realloc(table->named, (table->named_count + 1) * sizeof(*named))))
    {
        IDispatch_Release(dispatch);
        return E_OUTOFMEMORY;
    }
    table->named = named;
    named[table->named_count].name = wcsdup(name);
    named[table->named_count].dispatch = dispatch;
    table->named_count++;
    return S_OK;
}

HRESULT host_objects_remove(struct host_objects *table, const WCHAR *name)
{
    UINT32 i;

    if (!name) return E_INVALIDARG;
    for (i = 0; table && i < table->named_count; i++)
    {
        if (wcscmp(table->named[i].name, name)) continue;
        IDispatch_Release(table->named[i].dispatch);
        free(table->named[i].name);
        table->named[i] = table->named[--table->named_count];
        return S_OK;
    }
    return S_OK;
}

void host_objects_free(struct host_objects *table)
{
    UINT32 i;

    if (!table) return;
    for (i = 0; i < table->named_count; i++)
    {
        IDispatch_Release(table->named[i].dispatch);
        free(table->named[i].name);
    }
    for (i = 0; i < table->object_count; i++)
        if (table->objects[i]) IDispatch_Release(table->objects[i]);
    free(table->named);
    free(table->objects);
    free(table);
}

static UINT32 register_object(struct host_objects *table, IDispatch *dispatch)
{
    IDispatch **objects;
    UINT32 i;

    for (i = 0; i < table->object_count; i++)
        if (table->objects[i] == dispatch) return i + 1;
    if (!(objects = realloc(table->objects, (table->object_count + 1) * sizeof(*objects)))) return 0;
    table->objects = objects;
    IDispatch_AddRef(objects[table->object_count] = dispatch);
    return ++table->object_count;
}

/* Values (WV2_VALUE_*) <-> VARIANT. */

static HRESULT read_value(struct host_objects *table, struct wv2_reader *rd, VARIANT *v, int depth)
{
    UINT32 tag = wv2_get_u32(rd), id, count, i;
    SAFEARRAY *array;
    VARIANT *items;
    WCHAR *str;
    HRESULT hr;

    VariantInit(v);
    if (depth > 32) return E_INVALIDARG;
    switch (tag)
    {
    case WV2_VALUE_UNDEFINED: return S_OK;
    case WV2_VALUE_NULL: V_VT(v) = VT_NULL; return S_OK;
    case WV2_VALUE_BOOL: V_VT(v) = VT_BOOL; V_BOOL(v) = wv2_get_u32(rd) ? VARIANT_TRUE : VARIANT_FALSE; return S_OK;
    case WV2_VALUE_INT: V_VT(v) = VT_I4; V_I4(v) = (INT32)wv2_get_u32(rd); return S_OK;
    case WV2_VALUE_DOUBLE: V_VT(v) = VT_R8; V_R8(v) = wv2_get_f64(rd); return S_OK;
    case WV2_VALUE_STRING:
        str = wv2_get_str(rd);
        V_VT(v) = VT_BSTR;
        V_BSTR(v) = SysAllocString(str ? str : L"");
        free(str);
        return V_BSTR(v) ? S_OK : E_OUTOFMEMORY;
    case WV2_VALUE_OBJECT:
        id = wv2_get_u32(rd);
        if (!id || id > table->object_count || !table->objects[id - 1]) return E_INVALIDARG;
        V_VT(v) = VT_DISPATCH;
        IDispatch_AddRef(V_DISPATCH(v) = table->objects[id - 1]);
        return S_OK;
    case WV2_VALUE_ARRAY:
        count = wv2_get_u32(rd);
        if (count > 1000000 || !(array = SafeArrayCreateVector(VT_VARIANT, 0, count))) return E_OUTOFMEMORY;
        SafeArrayAccessData(array, (void **)&items);
        for (i = 0, hr = S_OK; i < count && SUCCEEDED(hr); i++) hr = read_value(table, rd, &items[i], depth + 1);
        SafeArrayUnaccessData(array);
        if (FAILED(hr))
        {
            SafeArrayDestroy(array);
            return hr;
        }
        V_VT(v) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(v) = array;
        return S_OK;
    }
    return E_INVALIDARG;
}

static void write_value(struct host_objects *table, struct wv2_buffer *buf, VARIANT *value, int depth)
{
    VARIANT v, converted;
    IDispatch *dispatch;
    LONG lower, upper, i;

    VariantInit(&v);
    VariantInit(&converted);
    if (FAILED(VariantCopyInd(&v, value)) || depth > 32)
    {
        wv2_put_u32(buf, WV2_VALUE_UNDEFINED);
        return;
    }
    switch (V_VT(&v))
    {
    case VT_EMPTY: wv2_put_u32(buf, WV2_VALUE_UNDEFINED); break;
    case VT_NULL: wv2_put_u32(buf, WV2_VALUE_NULL); break;
    case VT_BOOL: wv2_put_u32(buf, WV2_VALUE_BOOL); wv2_put_u32(buf, V_BOOL(&v) != VARIANT_FALSE); break;
    case VT_I1: case VT_I2: case VT_I4: case VT_INT: case VT_UI1: case VT_UI2:
        VariantChangeType(&converted, &v, 0, VT_I4);
        wv2_put_u32(buf, WV2_VALUE_INT);
        wv2_put_u32(buf, V_I4(&converted));
        break;
    case VT_UI4: case VT_UINT: case VT_I8: case VT_UI8: case VT_R4: case VT_R8: case VT_CY: case VT_DECIMAL:
        VariantChangeType(&converted, &v, 0, VT_R8);
        wv2_put_u32(buf, WV2_VALUE_DOUBLE);
        wv2_put_f64(buf, V_R8(&converted));
        break;
    case VT_BSTR:
        wv2_put_u32(buf, WV2_VALUE_STRING);
        wv2_put_str(buf, V_BSTR(&v) ? V_BSTR(&v) : L"");
        break;
    case VT_DISPATCH:
    case VT_UNKNOWN:
        if ((dispatch = dispatch_from_variant(&v)))
        {
            wv2_put_u32(buf, WV2_VALUE_OBJECT);
            wv2_put_u32(buf, register_object(table, dispatch));
            IDispatch_Release(dispatch);
        }
        else wv2_put_u32(buf, WV2_VALUE_NULL);
        break;
    default:
        if ((V_VT(&v) & VT_ARRAY) && SafeArrayGetDim(V_ARRAY(&v)) == 1)
        {
            VARTYPE vt = V_VT(&v) & VT_TYPEMASK;
            SafeArrayGetLBound(V_ARRAY(&v), 1, &lower);
            SafeArrayGetUBound(V_ARRAY(&v), 1, &upper);
            wv2_put_u32(buf, WV2_VALUE_ARRAY);
            wv2_put_u32(buf, upper >= lower ? upper - lower + 1 : 0);
            for (i = lower; i <= upper; i++)
            {
                VARIANT item;
                VariantInit(&item);
                if (vt == VT_VARIANT) SafeArrayGetElement(V_ARRAY(&v), &i, &item);
                else if (vt == VT_DECIMAL)
                {
                    /* DECIMAL overlays the whole VARIANT (its first field is the VARTYPE). */
                    SafeArrayGetElement(V_ARRAY(&v), &i, &V_DECIMAL(&item));
                    V_VT(&item) = VT_DECIMAL;
                }
                else if (vt != VT_RECORD)
                {
                    V_VT(&item) = vt;
                    SafeArrayGetElement(V_ARRAY(&v), &i, &V_BYREF(&item)); /* scalars, BSTR, interfaces */
                }
                write_value(table, buf, &item, depth + 1);
                VariantClear(&item);
            }
        }
        else if (SUCCEEDED(VariantChangeType(&converted, &v, 0, VT_BSTR)))  /* VT_DATE and others */
        {
            wv2_put_u32(buf, WV2_VALUE_STRING);
            wv2_put_str(buf, V_BSTR(&converted) ? V_BSTR(&converted) : L"");
        }
        else
        {
            FIXME("unsupported result type %#x\n", V_VT(&v));
            wv2_put_u32(buf, WV2_VALUE_UNDEFINED);
        }
        break;
    }
    VariantClear(&converted);
    VariantClear(&v);
}

static HRESULT get_dispid(IDispatch *dispatch, WCHAR *name, DISPID *id)
{
    return IDispatch_GetIDsOfNames(dispatch, &IID_NULL, &name, 1, LOCALE_USER_DEFAULT, id);
}

static WCHAR *error_message(HRESULT hr, EXCEPINFO *info, const WCHAR *member)
{
    WCHAR *message = malloc(512 * sizeof(WCHAR));

    if (!message) return NULL;
    if (info && info->bstrDescription && *info->bstrDescription)
        swprintf(message, 512, L"%s", info->bstrDescription);
    else if (hr == DISP_E_UNKNOWNNAME)
        swprintf(message, 512, L"Element not found: %s", member ? member : L"");
    else
        swprintf(message, 512, L"Host object call failed (0x%08lx)", hr);
    return message;
}

/* The object at the end of `path` minus its last member. */
static HRESULT walk(IDispatch *root, WCHAR **path, UINT32 count, IDispatch **out, WCHAR **failed)
{
    IDispatch *current = root, *next;
    DISPPARAMS none = {0};
    VARIANT result;
    DISPID id;
    HRESULT hr;
    UINT32 i;

    IDispatch_AddRef(current);
    for (i = 0; i + 1 < count; i++)
    {
        *failed = path[i];
        if (FAILED(hr = get_dispid(current, path[i], &id)))
        {
            IDispatch_Release(current);
            return hr;
        }
        VariantInit(&result);
        hr = IDispatch_Invoke(current, id, &IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &none, &result, NULL, NULL);
        IDispatch_Release(current);
        if (FAILED(hr)) return hr;
        next = dispatch_from_variant(&result);
        VariantClear(&result);
        if (!(current = next)) return E_INVALIDARG;
    }
    *out = current;
    return S_OK;
}

/* Runs one WV2_EVT_HOST_OBJECT_CALL and builds the WV2_CMD_HOST_OBJECT_REPLY. */
void host_objects_call(struct host_objects *table, const void *payload, UINT32 size, struct wv2_buffer *reply)
{
    struct wv2_reader rd = {payload, size, 0, NULL};
    WCHAR *name = NULL, **path = NULL, *failed = NULL, *message;
    IDispatch *root = NULL, *target = NULL;
    VARIANT *args = NULL, result;
    DISPPARAMS params = {0};
    EXCEPINFO info = {0};
    DISPID id, put_id = DISPID_PROPERTYPUT;
    UINT32 op, root_id, count, argc = 0, i;
    struct host_objects empty = {0};
    UINT64 call_id;
    HRESULT hr = S_OK;

    if (!table) table = &empty;
    VariantInit(&result);
    call_id = wv2_get_u64(&rd);
    op = wv2_get_u32(&rd);
    root_id = wv2_get_u32(&rd);
    if (!root_id) name = wv2_get_str(&rd);
    count = wv2_get_u32(&rd);
    if (count > 256 || !(path = calloc(count + 1, sizeof(*path)))) { hr = E_INVALIDARG; goto done; }
    for (i = 0; i < count; i++) path[i] = wv2_get_str(&rd);
    argc = wv2_get_u32(&rd);
    if (argc > 256 || !(args = calloc(argc + 1, sizeof(*args)))) { argc = 0; hr = E_INVALIDARG; goto done; }
    /* DISPPARAMS takes the arguments in reverse order. */
    for (i = 0; i < argc && SUCCEEDED(hr); i++) hr = read_value(table, &rd, &args[argc - 1 - i], 0);
    if (FAILED(hr)) goto done;

    if (op == WV2_HOST_RELEASE)
    {
        if (root_id && root_id <= table->object_count && table->objects[root_id - 1])
        {
            IDispatch_Release(table->objects[root_id - 1]);
            table->objects[root_id - 1] = NULL;
        }
        goto done;
    }
    if (root_id)
    {
        if (root_id <= table->object_count && (root = table->objects[root_id - 1])) IDispatch_AddRef(root);
    }
    else
    {
        for (i = 0; i < table->named_count; i++)
            if (name && !wcscmp(table->named[i].name, name)) IDispatch_AddRef(root = table->named[i].dispatch);
    }
    TRACE("call %s op %u root %u %s, %u members, %u args\n", wine_dbgstr_longlong(call_id), op, root_id,
          debugstr_w(name), count, argc);
    if (!root)
    {
        hr = DISP_E_UNKNOWNNAME;
        failed = name;
        goto done;
    }
    if (!count)
    {
        /* The object itself (e.g. `await hostObjects.name` resolves to the object). */
        V_VT(&result) = VT_DISPATCH;
        IDispatch_AddRef(V_DISPATCH(&result) = root);
        goto done;
    }
    if (FAILED(hr = walk(root, path, count, &target, &failed))) goto done;
    failed = path[count - 1];
    if (FAILED(hr = get_dispid(target, path[count - 1], &id))) goto done;

    params.rgvarg = args;
    params.cArgs = argc;
    switch (op)
    {
    case WV2_HOST_GET:
        params.cArgs = 0;
        hr = IDispatch_Invoke(target, id, &IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &params, &result, &info, NULL);
        break;
    case WV2_HOST_SET:
        params.cArgs = min(argc, 1);
        params.cNamedArgs = 1;
        params.rgdispidNamedArgs = &put_id;
        hr = IDispatch_Invoke(target, id, &IID_NULL, LOCALE_USER_DEFAULT,
                              (argc && V_VT(&args[0]) == VT_DISPATCH) ? DISPATCH_PROPERTYPUTREF : DISPATCH_PROPERTYPUT,
                              &params, NULL, &info, NULL);
        break;
    case WV2_HOST_CALL:
        hr = IDispatch_Invoke(target, id, &IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                              &params, &result, &info, NULL);
        break;
    default:
        hr = E_INVALIDARG;
    }

done:
    wv2_put_u64(reply, call_id);
    wv2_put_u32(reply, SUCCEEDED(hr));
    if (SUCCEEDED(hr)) write_value(table, reply, &result, 0);
    else
    {
        message = error_message(hr, &info, failed);
        TRACE("call failed: %#lx %s\n", hr, debugstr_w(message));
        wv2_put_str(reply, message ? message : L"Host object call failed");
        free(message);
    }
    SysFreeString(info.bstrSource);
    SysFreeString(info.bstrDescription);
    SysFreeString(info.bstrHelpFile);
    VariantClear(&result);
    for (i = 0; i < argc; i++) VariantClear(&args[i]);
    free(args);
    for (i = 0; path && i < count; i++) free(path[i]);
    free(path);
    free(name);
    if (target) IDispatch_Release(target);
    if (root) IDispatch_Release(root);
}
