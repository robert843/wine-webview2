// Test: AddHostObjectToScript. A small IDispatch object called from script through
// chrome.webview.hostObjects: methods, property get/set, nested objects, arrays, objects passed
// back, exceptions and unknown names. The script reports through postMessage.
#include <windows.h>
#include <oleauto.h>
#include <cstdio>
#include <functional>
#include <string>
#include "WebView2.h"

template <typename I, typename... A>
struct Handler : I {
    std::function<HRESULT(A...)> fn;
    LONG ref = 1;
    explicit Handler(std::function<HRESULT(A...)> f) : fn(f) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **ppv) override { *ppv = this; AddRef(); return S_OK; }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&ref); }
    ULONG STDMETHODCALLTYPE Release() override { LONG r = InterlockedDecrement(&ref); if (!r) delete this; return r; }
    HRESULT STDMETHODCALLTYPE Invoke(A... a) override { return fn(a...); }
};

static void log(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf("\n"); fflush(stdout);
}

static DWORD ui_thread;

struct Calc : IDispatch {
    LONG ref = 1;
    std::wstring name;
    explicit Calc(const wchar_t *n) : name(n) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppv) override {
        if (iid == IID_IUnknown || iid == IID_IDispatch) { *ppv = this; AddRef(); return S_OK; }
        *ppv = nullptr; return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&ref); }
    ULONG STDMETHODCALLTYPE Release() override { LONG r = InterlockedDecrement(&ref); if (!r) delete this; return r; }
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *n) override { *n = 0; return S_OK; }
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT, LCID, ITypeInfo **) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID, LPOLESTR *names, UINT count, LCID, DISPID *ids) override {
        static const wchar_t *known[] = {L"Add", L"Name", L"Child", L"Echo", L"Fail", L"NameOf"};
        for (UINT i = 0; i < count; i++) {
            ids[i] = DISPID_UNKNOWN;
            for (int k = 0; k < 6; k++) if (!wcscmp(names[i], known[k])) ids[i] = k + 1;
            if (ids[i] == DISPID_UNKNOWN) return DISP_E_UNKNOWNNAME;
        }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Invoke(DISPID id, REFIID, LCID, WORD flags, DISPPARAMS *p, VARIANT *result,
                                     EXCEPINFO *info, UINT *) override {
        if (GetCurrentThreadId() != ui_thread) log("called on the wrong thread");
        VARIANT tmp; VariantInit(&tmp);
        switch (id) {
        case 1: { // Add(a, b): arguments arrive in reverse order
            if (p->cArgs != 2) return DISP_E_BADPARAMCOUNT;
            VARIANT a, b; VariantInit(&a); VariantInit(&b);
            VariantChangeType(&a, &p->rgvarg[1], 0, VT_I4); VariantChangeType(&b, &p->rgvarg[0], 0, VT_I4);
            V_VT(result) = VT_I4; V_I4(result) = V_I4(&a) + V_I4(&b);
            return S_OK; }
        case 2: // Name
            if (flags & DISPATCH_PROPERTYPUT) {
                VariantChangeType(&tmp, &p->rgvarg[0], 0, VT_BSTR);
                name = V_BSTR(&tmp); VariantClear(&tmp);
                return S_OK;
            }
            V_VT(result) = VT_BSTR; V_BSTR(result) = SysAllocString(name.c_str());
            return S_OK;
        case 3: // Child: a new object each time
            V_VT(result) = VT_DISPATCH; V_DISPATCH(result) = new Calc(L"child");
            return S_OK;
        case 4: // Echo(x)
            return VariantCopy(result, &p->rgvarg[0]);
        case 5: // Fail()
            if (info) { memset(info, 0, sizeof(*info)); info->bstrDescription = SysAllocString(L"boom"); info->scode = E_FAIL; }
            return DISP_E_EXCEPTION;
        case 6: { // NameOf(object)
            if (p->cArgs != 1 || V_VT(&p->rgvarg[0]) != VT_DISPATCH) return DISP_E_TYPEMISMATCH;
            Calc *other = static_cast<Calc *>(V_DISPATCH(&p->rgvarg[0]));
            V_VT(result) = VT_BSTR; V_BSTR(result) = SysAllocString(other->name.c_str());
            return S_OK; }
        }
        return DISP_E_MEMBERNOTFOUND;
    }
};

static ICoreWebView2 *webview;
static ICoreWebView2Controller *controller;
static HWND hwnd;

static const wchar_t script[] =
    L"(async () => {"
    L"  const calc = chrome.webview.hostObjects.calc, r = [];"
    L"  r.push('add=' + await calc.Add(2, 3));"
    L"  r.push('name=' + await calc.Name);"
    L"  calc.Name = 'renamed';"
    L"  r.push('renamed=' + await calc.Name);"
    L"  r.push('chain=' + await calc.Child.Add(10, 1));"
    L"  const child = await calc.Child;"
    L"  r.push('childname=' + await child.Name);"
    L"  r.push('echo=' + JSON.stringify(await calc.Echo([1, 'two', true, null, 2.5])));"
    L"  r.push('nameof=' + await calc.NameOf(child));"
    L"  try { await calc.Fail(); r.push('fail=none'); } catch (e) { r.push('fail=' + e.message); }"
    L"  try { await calc.Missing(); r.push('missing=none'); } catch (e) { r.push('missing=' + e.message); }"
    L"  try { await chrome.webview.hostObjects.nothere.Foo(); } catch (e) { r.push('nothere=' + e.message); }"
    L"  chrome.webview.postMessage(r.join(';'));"
    L"})().catch(e => chrome.webview.postMessage('error ' + e));";

static LRESULT CALLBACK wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, msg, wp, lp);
}

static void CALLBACK on_timeout(HWND, UINT, UINT_PTR, DWORD) { log("timeout"); PostQuitMessage(2); }

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    ui_thread = GetCurrentThreadId();
    WNDCLASSW wc = {}; wc.lpfnWndProc = wndproc; wc.hInstance = inst; wc.lpszClassName = L"hosttest";
    RegisterClassW(&wc);
    hwnd = CreateWindowW(L"hosttest", L"wine-webview2 hosttest", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                         100, 100, 640, 400, nullptr, nullptr, inst, nullptr);
    auto create = (HRESULT (STDMETHODCALLTYPE *)(PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions *,
                   ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *))
        GetProcAddress(LoadLibraryW(L"WebView2Loader.dll"), "CreateCoreWebView2EnvironmentWithOptions");
    create(nullptr, nullptr, nullptr,
        new Handler<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, HRESULT, ICoreWebView2Environment *>(
            [](HRESULT, ICoreWebView2Environment *env) {
                env->CreateCoreWebView2Controller(hwnd,
                    new Handler<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, HRESULT, ICoreWebView2Controller *>(
                        [](HRESULT, ICoreWebView2Controller *c) {
                            controller = c; controller->AddRef();
                            controller->get_CoreWebView2(&webview);
                            RECT r = {0, 0, 640, 360}; controller->put_Bounds(r);
                            VARIANT object; V_VT(&object) = VT_DISPATCH; V_DISPATCH(&object) = new Calc(L"calc");
                            log("AddHostObjectToScript hr=%#lx", webview->AddHostObjectToScript(L"calc", &object));
                            VariantClear(&object);
                            EventRegistrationToken t;
                            webview->add_WebMessageReceived(new Handler<ICoreWebView2WebMessageReceivedEventHandler, ICoreWebView2 *, ICoreWebView2WebMessageReceivedEventArgs *>(
                                [](ICoreWebView2 *, ICoreWebView2WebMessageReceivedEventArgs *args) {
                                    LPWSTR text = nullptr; args->TryGetWebMessageAsString(&text);
                                    char utf8[2048]; WideCharToMultiByte(CP_UTF8, 0, text ? text : L"", -1, utf8, sizeof(utf8), nullptr, nullptr);
                                    log("result %s", utf8);
                                    CoTaskMemFree(text);
                                    log("done"); PostMessageW(hwnd, WM_CLOSE, 0, 0);
                                    return S_OK; }), &t);
                            static bool started;
                            webview->add_NavigationCompleted(new Handler<ICoreWebView2NavigationCompletedEventHandler, ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *>(
                                [](ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *) {
                                    if (!started) { started = true; webview->ExecuteScript(script, nullptr); } return S_OK; }), &t);
                            webview->NavigateToString(L"<body>host objects</body>");
                            return S_OK; }));
                return S_OK; }));
    SetTimer(nullptr, 0, 40000, on_timeout);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}
