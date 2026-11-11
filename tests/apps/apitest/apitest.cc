// Test: profile (name, PreferredColorScheme), ZoomFactor + ZoomFactorChanged, GotFocus,
// environment UserDataFolder, controller options.
#include <windows.h>
#include <cstdio>
#include <functional>
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

static const GUID iid_webview13 = {0xf75f09a8, 0x667e, 0x4983, {0x88, 0xd6, 0xc8, 0x77, 0x3f, 0x31, 0x5e, 0x84}};
static const GUID iid_env7 = {0x43c22296, 0x3bbd, 0x43a4, {0x9c, 0x00, 0x5c, 0x0d, 0xf6, 0xdd, 0x29, 0xa2}};
static const GUID iid_env10 = {0xee0eb9df, 0x6f12, 0x46ce, {0xb5, 0x3f, 0x3f, 0x47, 0xb9, 0xc9, 0x28, 0xe0}};

static ICoreWebView2Environment *env;
static ICoreWebView2 *webview;
static ICoreWebView2Controller *controller;
static HWND hwnd;

static void log(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf("\n"); fflush(stdout);
}

static void step(int n);

static void run(const wchar_t *script, int next) {
    webview->ExecuteScript(script, new Handler<ICoreWebView2ExecuteScriptCompletedHandler, HRESULT, LPCWSTR>(
        [script, next](HRESULT hr, LPCWSTR json) {
            log("script %ls -> %ls", script, json ? json : L"(null)");
            if (next) step(next);
            return S_OK; }));
}

static void later(int ms, int n) {
    static int pending;
    pending = n;
    SetTimer(hwnd, 10, ms, [](HWND h, UINT, UINT_PTR id, DWORD) { KillTimer(h, id); step(pending); });
}

static void step(int n) {
    log("--- step %d", n);
    switch (n) {
    case 1: {
        ICoreWebView2Environment7 *e7;
        if (SUCCEEDED(env->QueryInterface(iid_env7, (void **)&e7))) {
            LPWSTR udf = nullptr; e7->get_UserDataFolder(&udf);
            log("UserDataFolder set=%d", udf && *udf);
            CoTaskMemFree(udf); e7->Release();
        }
        ICoreWebView2Environment10 *e10;
        if (SUCCEEDED(env->QueryInterface(iid_env10, (void **)&e10))) {
            ICoreWebView2ControllerOptions *o = nullptr;
            e10->CreateCoreWebView2ControllerOptions(&o);
            if (o) {
                o->put_ProfileName(L"apitest"); o->put_IsInPrivateModeEnabled(TRUE);
                LPWSTR name = nullptr; BOOL priv = FALSE;
                o->get_ProfileName(&name); o->get_IsInPrivateModeEnabled(&priv);
                log("ControllerOptions name=%ls private=%d", name ? name : L"?", priv);
                CoTaskMemFree(name); o->Release();
            }
            e10->Release();
        }
        ICoreWebView2_13 *w13;
        if (SUCCEEDED(webview->QueryInterface(iid_webview13, (void **)&w13))) {
            ICoreWebView2Profile *p = nullptr;
            w13->get_Profile(&p);
            if (p) {
                LPWSTR name = nullptr; p->get_ProfileName(&name);
                log("Profile name=%ls", name ? name : L"?");
                CoTaskMemFree(name);
                log("put_PreferredColorScheme(dark) hr=%#lx", p->put_PreferredColorScheme(COREWEBVIEW2_PREFERRED_COLOR_SCHEME_DARK));
                p->Release();
            }
            w13->Release();
        }
        later(500, 2); break; }
    case 2: run(L"'dark=' + matchMedia('(prefers-color-scheme: dark)').matches", 3); break;
    case 3: {
        EventRegistrationToken t;
        controller->add_ZoomFactorChanged(new Handler<ICoreWebView2ZoomFactorChangedEventHandler, ICoreWebView2Controller *, IUnknown *>(
            [](ICoreWebView2Controller *c, IUnknown *) { double z = 0; c->get_ZoomFactor(&z); log("ZoomFactorChanged %.2f", z); return S_OK; }), &t);
        log("put_ZoomFactor(1.5) hr=%#lx", controller->put_ZoomFactor(1.5));
        later(1000, 4); break; }
    case 4: run(L"'dpr=' + window.devicePixelRatio", 5); break;
    case 5: {
        EventRegistrationToken t;
        controller->add_GotFocus(new Handler<ICoreWebView2FocusChangedEventHandler, ICoreWebView2Controller *, IUnknown *>(
            [](ICoreWebView2Controller *, IUnknown *) { log("GotFocus"); return S_OK; }), &t);
        SetFocus(hwnd);
        controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        later(1000, 6); break; }
    case 6: log("done"); PostMessageW(hwnd, WM_CLOSE, 0, 0); break;
    }
}

static LRESULT CALLBACK wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, msg, wp, lp);
}

static void CALLBACK on_timeout(HWND, UINT, UINT_PTR, DWORD) { log("timeout"); PostQuitMessage(2); }

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSW wc = {}; wc.lpfnWndProc = wndproc; wc.hInstance = inst; wc.lpszClassName = L"apitest";
    RegisterClassW(&wc);
    hwnd = CreateWindowW(L"apitest", L"wine-webview2 apitest", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                         100, 100, 640, 400, nullptr, nullptr, inst, nullptr);
    auto create = (HRESULT (STDMETHODCALLTYPE *)(PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions *,
                   ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *))
        GetProcAddress(LoadLibraryW(L"WebView2Loader.dll"), "CreateCoreWebView2EnvironmentWithOptions");
    create(nullptr, nullptr, nullptr,
        new Handler<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, HRESULT, ICoreWebView2Environment *>(
            [](HRESULT, ICoreWebView2Environment *e) {
                env = e; env->AddRef();
                env->CreateCoreWebView2Controller(hwnd,
                    new Handler<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, HRESULT, ICoreWebView2Controller *>(
                        [](HRESULT, ICoreWebView2Controller *c) {
                            controller = c; controller->AddRef();
                            controller->get_CoreWebView2(&webview);
                            RECT r = {0, 0, 640, 360}; controller->put_Bounds(r);
                            EventRegistrationToken t;
                            static bool started;
                            webview->add_NavigationCompleted(new Handler<ICoreWebView2NavigationCompletedEventHandler, ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *>(
                                [](ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *) {
                                    if (!started) { started = true; later(300, 1); } return S_OK; }), &t);
                            webview->NavigateToString(L"<title>apitest</title><body>apitest</body>");
                            return S_OK; }));
                return S_OK; }));
    SetTimer(nullptr, 0, 40000, on_timeout);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}
