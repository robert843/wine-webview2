// Test: przezroczyste DefaultBackgroundColor, Settings (UserAgent, IsWebMessageEnabled), ExecuteScript.
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

// IID_ICoreWebView2Controller2 (uuid z WebView2.idl)
static const GUID iid_controller2 = {0xc979903e, 0xd4ca, 0x4228, {0x92, 0xeb, 0x47, 0xee, 0x3f, 0xa9, 0x6e, 0xab}};
static ICoreWebView2 *webview;
static ICoreWebView2Controller *controller;
static HWND hwnd;
static int step;

static void log(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf("\n"); fflush(stdout);
}

static const wchar_t page[] = L"<html><body style='background:transparent;margin:0;font:28px sans-serif'>"
    L"<div style='margin:40px;padding:30px;background:rgba(0,80,255,0.5);color:white'>półprzezroczysty</div>"
    L"<p style='margin:40px'>tekst na przezroczystym tle</p></body></html>";

static void probe() {
    webview->ExecuteScript(L"navigator.userAgent + ' | ' + navigator.platform + ' | webview=' + typeof (window.chrome && window.chrome.webview)",
        new Handler<ICoreWebView2ExecuteScriptCompletedHandler, HRESULT, LPCWSTR>([](HRESULT hr, LPCWSTR json) {
            log("step %d probe hr=%#lx result=%ls", step, hr, json ? json : L"(null)");
            if (step == 1) {
                ICoreWebView2Settings *s; webview->get_Settings(&s);
                s->put_IsWebMessageEnabled(FALSE); s->Release();
                step = 2; webview->NavigateToString(page);
            } else {
                // DevTools: wywołanie metody + odbiornik zdarzenia konsoli
                webview->CallDevToolsProtocolMethod(L"Browser.getVersion", L"{}",
                    new Handler<ICoreWebView2CallDevToolsProtocolMethodCompletedHandler, HRESULT, LPCWSTR>([](HRESULT hr, LPCWSTR json) {
                        log("cdp Browser.getVersion hr=%#lx result=%.160ls", hr, json ? json : L"(null)");
                        return S_OK; }));
                ICoreWebView2DevToolsProtocolEventReceiver *rcv = nullptr;
                webview->GetDevToolsProtocolEventReceiver(L"Runtime.consoleAPICalled", &rcv);
                if (rcv) {
                    EventRegistrationToken t;
                    rcv->add_DevToolsProtocolEventReceived(new Handler<ICoreWebView2DevToolsProtocolEventReceivedEventHandler, ICoreWebView2 *, ICoreWebView2DevToolsProtocolEventReceivedEventArgs *>(
                        [](ICoreWebView2 *, ICoreWebView2DevToolsProtocolEventReceivedEventArgs *a) {
                            LPWSTR p = nullptr; a->get_ParameterObjectAsJson(&p);
                            log("cdp event Runtime.consoleAPICalled: %.120ls", p ? p : L"");
                            CoTaskMemFree(p); return S_OK; }), &t);
                    webview->CallDevToolsProtocolMethod(L"Runtime.enable", L"{}",
                        new Handler<ICoreWebView2CallDevToolsProtocolMethodCompletedHandler, HRESULT, LPCWSTR>([](HRESULT hr, LPCWSTR) {
                            log("cdp Runtime.enable hr=%#lx", hr);
                            webview->ExecuteScript(L"console.log('cdp-test')", nullptr);
                            return S_OK; }));
                }
                SetTimer(hwnd, 1, 5000, [](HWND h, UINT, UINT_PTR, DWORD) { PostMessageW(h, WM_CLOSE, 0, 0); });
            }
            return S_OK; }));
}

static LRESULT CALLBACK wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_SIZE && controller) { RECT r; GetClientRect(h, &r); controller->put_Bounds(r); }
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSW wc = {}; wc.lpfnWndProc = wndproc; wc.hInstance = inst; wc.lpszClassName = L"bgtest";
    wc.hbrBackground = CreateSolidBrush(RGB(220, 30, 30));
    RegisterClassW(&wc);
    hwnd = CreateWindowW(L"bgtest", L"wine-webview2 bgtest", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                         100, 100, 600, 400, nullptr, nullptr, inst, nullptr);
    auto create = (HRESULT (STDMETHODCALLTYPE *)(PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions *,
                   ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *))
        GetProcAddress(LoadLibraryW(L"WebView2Loader.dll"), "CreateCoreWebView2EnvironmentWithOptions");
    create(nullptr, nullptr, nullptr,
        new Handler<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, HRESULT, ICoreWebView2Environment *>(
            [](HRESULT, ICoreWebView2Environment *env) {
                LPWSTR ver; env->get_BrowserVersionString(&ver); log("runtime %ls", ver); CoTaskMemFree(ver);
                env->CreateCoreWebView2Controller(hwnd,
                    new Handler<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, HRESULT, ICoreWebView2Controller *>(
                        [](HRESULT, ICoreWebView2Controller *c) {
                            controller = c; controller->AddRef();
                            ICoreWebView2Controller2 *c2;
                            if (SUCCEEDED(controller->QueryInterface(iid_controller2, (void **)&c2))) {
                                COREWEBVIEW2_COLOR transparent = {0, 0, 0, 0};
                                log("put_DefaultBackgroundColor(transparent) hr=%#lx", c2->put_DefaultBackgroundColor(transparent));
                                c2->Release();
                            }
                            controller->get_CoreWebView2(&webview);
                            RECT r; GetClientRect(hwnd, &r); controller->put_Bounds(r);
                            ICoreWebView2Settings *s; webview->get_Settings(&s);
                            BOOL msgs, dev, menus; LPWSTR ua = nullptr;
                            s->get_IsWebMessageEnabled(&msgs); s->get_AreDevToolsEnabled(&dev); s->get_AreDefaultContextMenusEnabled(&menus);
                            log("settings: web_message=%d devtools=%d context_menus=%d", msgs, dev, menus);
                            s->Release();
                            EventRegistrationToken t;
                            webview->add_NavigationCompleted(new Handler<ICoreWebView2NavigationCompletedEventHandler, ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *>(
                                [](ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *) { Sleep(300); probe(); return S_OK; }), &t);
                            step = 1; webview->NavigateToString(page);
                            return S_OK; }));
                return S_OK; }));
    SetTimer(nullptr, 0, 25000, [](HWND, UINT, UINT_PTR, DWORD) { log("timeout"); PostQuitMessage(2); });
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}
