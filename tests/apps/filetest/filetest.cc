// Test: <input type=file> opens the application's common file dialog (GetOpenFileName), the chosen
// Windows path reaches the page as a readable file; cancelling keeps the page working.
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
static std::wstring pick_path; // <exe dir>\ft\pick.txt

static void log(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf("\n"); fflush(stdout);
}

static void step(int n);

static void run(const wchar_t *script, int next) {
    webview->ExecuteScript(script, new Handler<ICoreWebView2ExecuteScriptCompletedHandler, HRESULT, LPCWSTR>(
        [next](HRESULT hr, LPCWSTR json) {
            log("script -> hr=%#lx %ls", hr, json ? json : L"(null)");
            if (next) step(next);
            return S_OK; }));
}

static void later(int ms, int n) {
    static int pending;
    pending = n;
    SetTimer(hwnd, 10, ms, [](HWND h, UINT, UINT_PTR id, DWORD) { KillTimer(h, id); step(pending); });
}

static void click() {
    HWND child = FindWindowExW(hwnd, nullptr, nullptr, nullptr);
    PostMessageW(child, WM_MOUSEMOVE, 0, MAKELPARAM(60, 30));
    PostMessageW(child, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(60, 30));
    PostMessageW(child, WM_LBUTTONUP, 0, MAKELPARAM(60, 30));
}

// Waits for the file dialog, then types the path and presses OK, or cancels.
static bool choose;
static DWORD WINAPI dialog_driver(void *) {
    for (int i = 0; i < 100; i++) {
        Sleep(100);
        HWND dlg = FindWindowW(L"#32770", nullptr);
        if (!dlg || !IsWindowVisible(dlg)) continue;
        wchar_t title[128] = L""; GetWindowTextW(dlg, title, 128);
        char utf8[256]; WideCharToMultiByte(CP_UTF8, 0, title, -1, utf8, sizeof(utf8), nullptr, nullptr);
        log("file dialog shown: %s", utf8);
        Sleep(300);
        if (!choose) { PostMessageW(dlg, WM_COMMAND, IDCANCEL, 0); log("cancelled"); return 0; }
        // cmb13 (file name combo box) in the explorer-style dialog; edt1 in the old one.
        if (!SetDlgItemTextW(dlg, 1148, pick_path.c_str())) SetDlgItemTextW(dlg, 1152, pick_path.c_str());
        PostMessageW(dlg, WM_COMMAND, IDOK, 0);
        log("chose file");
        return 0;
    }
    log("file dialog: none");
    return 0;
}

static void start_driver(bool c) {
    choose = c;
    CloseHandle(CreateThread(nullptr, 0, dialog_driver, nullptr, 0, nullptr));
}

static void step(int n) {
    log("--- step %d", n);
    switch (n) {
    case 1: start_driver(true); click(); later(4000, 2); break;
    case 2: run(L"document.title", 3); break;
    case 3: start_driver(false); click(); later(3000, 4); break;
    case 4: run(L"'still alive ' + document.querySelector('input').files.length", 5); break;
    case 5: log("done"); PostMessageW(hwnd, WM_CLOSE, 0, 0); break;
    }
}

static LRESULT CALLBACK wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, msg, wp, lp);
}

static void CALLBACK on_timeout(HWND, UINT, UINT_PTR, DWORD) { log("timeout"); PostQuitMessage(2); }

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    wchar_t dir[MAX_PATH]; GetModuleFileNameW(nullptr, dir, MAX_PATH);
    *wcsrchr(dir, L'\\') = 0;
    std::wstring ft = std::wstring(dir) + L"\\ft";
    CreateDirectoryW(ft.c_str(), nullptr);
    pick_path = ft + L"\\pick.txt";
    HANDLE f = CreateFileW(pick_path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    DWORD written; WriteFile(f, "picked content", 14, &written, nullptr); CloseHandle(f);

    WNDCLASSW wc = {}; wc.lpfnWndProc = wndproc; wc.hInstance = inst; wc.lpszClassName = L"filetest";
    RegisterClassW(&wc);
    hwnd = CreateWindowW(L"filetest", L"wine-webview2 filetest", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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
                            EventRegistrationToken t;
                            static bool started;
                            webview->add_NavigationCompleted(new Handler<ICoreWebView2NavigationCompletedEventHandler, ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *>(
                                [](ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *) {
                                    if (!started) { started = true; later(500, 1); } return S_OK; }), &t);
                            webview->NavigateToString(
                                L"<body style='margin:0'><input type=file accept='.txt,text/plain' "
                                L"style='position:absolute;left:10px;top:10px;width:300px;height:40px'>"
                                L"<script>document.querySelector('input').onchange = e => {"
                                L"  var f = e.target.files[0]; f.text().then(t => document.title = f.name + ':' + t); };"
                                L"</script>");
                            return S_OK; }));
                return S_OK; }));
    SetTimer(nullptr, 0, 40000, on_timeout);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}
