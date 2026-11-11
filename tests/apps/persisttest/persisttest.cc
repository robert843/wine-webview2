// Test: browser data persists in the user data folder (<exe>.WebView2\EBWebView) between runs.
// Run twice: the second run sees the localStorage counter and the cookie of the first.
#include <windows.h>
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

static ICoreWebView2 *webview;
static ICoreWebView2Controller *controller;
static HWND hwnd;

static void log(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf("\n"); fflush(stdout);
}

static LRESULT CALLBACK wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, msg, wp, lp);
}

static void CALLBACK on_timeout(HWND, UINT, UINT_PTR, DWORD) { log("timeout"); PostQuitMessage(2); }

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    static wchar_t site[MAX_PATH]; GetModuleFileNameW(nullptr, site, MAX_PATH);
    wcscpy(wcsrchr(site, L'\\'), L"\\site");
    CreateDirectoryW(site, nullptr);
    HANDLE f = CreateFileW((std::wstring(site) + L"\\index.html").c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    DWORD n; WriteFile(f, "<body>persist</body>", 20, &n, nullptr); CloseHandle(f);

    WNDCLASSW wc = {}; wc.lpfnWndProc = wndproc; wc.hInstance = inst; wc.lpszClassName = L"persisttest";
    RegisterClassW(&wc);
    hwnd = CreateWindowW(L"persisttest", L"wine-webview2 persisttest", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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
                            ICoreWebView2_3 *w3; webview->QueryInterface(IID_ICoreWebView2_3, (void **)&w3);
                            w3->SetVirtualHostNameToFolderMapping(L"app.test", site, COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
                            w3->Release();
                            EventRegistrationToken t;
                            webview->add_NavigationCompleted(new Handler<ICoreWebView2NavigationCompletedEventHandler, ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *>(
                                [](ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *) {
                                    webview->ExecuteScript(
                                        L"var seen = /visited=yes/.test(document.cookie);"
                                        L"document.cookie = 'visited=yes; max-age=86400; path=/';"
                                        L"localStorage.n = (+localStorage.n || 0) + 1;"
                                        L"'n=' + localStorage.n + ' cookie=' + seen",
                                        new Handler<ICoreWebView2ExecuteScriptCompletedHandler, HRESULT, LPCWSTR>(
                                            [](HRESULT hr, LPCWSTR json) {
                                                log("script -> hr=%#lx %ls", hr, json ? json : L"(null)");
                                                // Give the browser time to flush the profile before closing.
                                                SetTimer(hwnd, 1, 1500, [](HWND h, UINT, UINT_PTR, DWORD) {
                                                    log("done"); PostMessageW(h, WM_CLOSE, 0, 0); });
                                                return S_OK; }));
                                    return S_OK; }), &t);
                            webview->Navigate(L"https://app.test/index.html");
                            return S_OK; }));
                return S_OK; }));
    SetTimer(nullptr, 0, 40000, on_timeout);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    controller->Close();
    return 0;
}
