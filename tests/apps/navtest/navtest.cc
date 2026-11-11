// WebView2 navigation event conformance test. Prints the event order to stdout; the same .exe
// must give the same sequence on Windows with the real runtime and under Wine with wine-webview2.
//   1. NavigateToString(A)  2. NavigateToString(B)  3. GoBack  4. done
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

// IID_ICoreWebView2_2 (uuid from WebView2.idl)
static const GUID iid_webview2_2 = {0x9E8F0CF8, 0xE670, 0x4B5E, {0xB2, 0xBC, 0x73, 0xE0, 0x61, 0xE3, 0x18, 0x4C}};
static ICoreWebView2 *webview;
static ICoreWebView2Controller *controller;
static int step;
static HWND hwnd;

static void log(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf("\n"); fflush(stdout);
}

static void show_state(const char *when) {
    LPWSTR src = nullptr, title = nullptr; BOOL back = 0, fwd = 0;
    webview->get_Source(&src); webview->get_DocumentTitle(&title);
    webview->get_CanGoBack(&back); webview->get_CanGoForward(&fwd);
    log("  [%s] Source=%.40ls Title=%ls CanGoBack=%d CanGoForward=%d", when, src ? src : L"", title ? title : L"", back, fwd);
    CoTaskMemFree(src); CoTaskMemFree(title);
}

static void next_step() {
    step++;
    log("== step %d", step);
    switch (step) {
    case 1: webview->NavigateToString(L"<title>Page A</title><h1>A</h1>"); break;
    case 2: webview->NavigateToString(L"<title>Page B</title><h1>B</h1>"); break;
    case 3: webview->GoBack(); break;
    default: log("== done"); PostMessageW(hwnd, WM_CLOSE, 0, 0); break;
    }
}

static void setup(ICoreWebView2 *wv) {
    EventRegistrationToken t;
    wv->add_NavigationStarting(new Handler<ICoreWebView2NavigationStartingEventHandler, ICoreWebView2 *, ICoreWebView2NavigationStartingEventArgs *>(
        [](ICoreWebView2 *, ICoreWebView2NavigationStartingEventArgs *a) {
            LPWSTR uri; UINT64 id; BOOL user, redir;
            a->get_Uri(&uri); a->get_NavigationId(&id); a->get_IsUserInitiated(&user); a->get_IsRedirected(&redir);
            log("NavigationStarting id=%llu user=%d redirect=%d uri=%.40ls", (unsigned long long)id, user, redir, uri);
            CoTaskMemFree(uri); return S_OK; }), &t);
    wv->add_SourceChanged(new Handler<ICoreWebView2SourceChangedEventHandler, ICoreWebView2 *, ICoreWebView2SourceChangedEventArgs *>(
        [](ICoreWebView2 *, ICoreWebView2SourceChangedEventArgs *a) {
            BOOL n; a->get_IsNewDocument(&n); log("SourceChanged new_document=%d", n); return S_OK; }), &t);
    wv->add_ContentLoading(new Handler<ICoreWebView2ContentLoadingEventHandler, ICoreWebView2 *, ICoreWebView2ContentLoadingEventArgs *>(
        [](ICoreWebView2 *, ICoreWebView2ContentLoadingEventArgs *a) {
            UINT64 id; BOOL err; a->get_NavigationId(&id); a->get_IsErrorPage(&err);
            log("ContentLoading id=%llu error_page=%d", (unsigned long long)id, err); return S_OK; }), &t);
    wv->add_HistoryChanged(new Handler<ICoreWebView2HistoryChangedEventHandler, ICoreWebView2 *, IUnknown *>(
        [](ICoreWebView2 *, IUnknown *) { log("HistoryChanged"); return S_OK; }), &t);
    ICoreWebView2_2 *wv2 = nullptr;
    if (FAILED(wv->QueryInterface(iid_webview2_2, (void **)&wv2))) { log("no ICoreWebView2_2"); return; }
    wv2->add_DOMContentLoaded(new Handler<ICoreWebView2DOMContentLoadedEventHandler, ICoreWebView2 *, ICoreWebView2DOMContentLoadedEventArgs *>(
        [](ICoreWebView2 *, ICoreWebView2DOMContentLoadedEventArgs *a) {
            UINT64 id; a->get_NavigationId(&id); log("DOMContentLoaded id=%llu", (unsigned long long)id); return S_OK; }), &t);
    wv->add_DocumentTitleChanged(new Handler<ICoreWebView2DocumentTitleChangedEventHandler, ICoreWebView2 *, IUnknown *>(
        [](ICoreWebView2 *, IUnknown *) { log("DocumentTitleChanged"); return S_OK; }), &t);
    wv->add_NavigationCompleted(new Handler<ICoreWebView2NavigationCompletedEventHandler, ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *>(
        [](ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *a) {
            UINT64 id; BOOL ok; COREWEBVIEW2_WEB_ERROR_STATUS st;
            a->get_NavigationId(&id); a->get_IsSuccess(&ok); a->get_WebErrorStatus(&st);
            log("NavigationCompleted id=%llu success=%d web_error=%d", (unsigned long long)id, ok, (int)st);
            show_state("after completed");
            next_step(); return S_OK; }), &t);
}

static LRESULT CALLBACK wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_SIZE && controller) { RECT r; GetClientRect(h, &r); controller->put_Bounds(r); }
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSW wc = {}; wc.lpfnWndProc = wndproc; wc.hInstance = inst; wc.lpszClassName = L"navtest";
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    RegisterClassW(&wc);
    hwnd = CreateWindowW(L"navtest", L"wine-webview2 navtest", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                         100, 100, 640, 480, nullptr, nullptr, inst, nullptr);
    HMODULE loader = LoadLibraryW(L"WebView2Loader.dll");
    auto create = (HRESULT (STDMETHODCALLTYPE *)(PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions *,
                   ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *))
        GetProcAddress(loader, "CreateCoreWebView2EnvironmentWithOptions");
    if (!create) { log("no WebView2Loader.dll"); return 1; }
    HRESULT hr = create(nullptr, nullptr, nullptr,
        new Handler<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, HRESULT, ICoreWebView2Environment *>(
            [](HRESULT hr, ICoreWebView2Environment *env) {
                log("environment created hr=%#lx", hr);
                if (FAILED(hr)) { PostQuitMessage(1); return S_OK; }
                env->CreateCoreWebView2Controller(hwnd,
                    new Handler<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, HRESULT, ICoreWebView2Controller *>(
                        [](HRESULT hr, ICoreWebView2Controller *c) {
                            log("controller created hr=%#lx", hr);
                            if (FAILED(hr)) { PostQuitMessage(1); return S_OK; }
                            controller = c; controller->AddRef();
                            controller->get_CoreWebView2(&webview);
                            RECT r; GetClientRect(hwnd, &r); controller->put_Bounds(r);
                            setup(webview);
                            show_state("initial");
                            next_step();
                            return S_OK; }));
                return S_OK; }));
    log("CreateCoreWebView2EnvironmentWithOptions hr=%#lx", hr);
    MSG msg;
    SetTimer(nullptr, 0, 20000, [](HWND, UINT, UINT_PTR, DWORD) { log("== timeout"); PostQuitMessage(2); });
    while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    if (controller) controller->Close();
    return 0;
}
