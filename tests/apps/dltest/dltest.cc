// Test: downloads. DownloadStarting with a custom ResultFilePath + StateChanged/BytesReceivedChanged,
// Cancel, and a deferral with the default path (the user's Downloads folder). The second and third
// downloads ask for MULTIPLE_AUTOMATIC_DOWNLOADS (PermissionRequested).
#include <windows.h>
#include <shlobj.h>
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
static ICoreWebView2_4 *webview4;
static ICoreWebView2Controller *controller;
static HWND hwnd;
static int mode;                 // what the next DownloadStarting does: 1 custom path, 2 cancel, 3 deferral
static std::wstring custom_path; // <exe dir>\dl\custom.txt
static std::wstring default_path;
static ICoreWebView2Deferral *deferral;

static void log(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf("\n"); fflush(stdout);
}

static std::string utf8(const wchar_t *s) {
    char buf[1024];
    WideCharToMultiByte(CP_UTF8, 0, s ? s : L"", -1, buf, sizeof(buf), nullptr, nullptr);
    return buf;
}

static std::string read_file(const std::wstring &path) {
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (f == INVALID_HANDLE_VALUE) return "(missing)";
    char buf[256] = ""; DWORD got = 0;
    ReadFile(f, buf, sizeof(buf) - 1, &got, nullptr);
    CloseHandle(f);
    return std::string(buf, got);
}

static void step(int n);

static void later(int ms, int n) {
    static int pending;
    pending = n;
    SetTimer(hwnd, 10, ms, [](HWND h, UINT, UINT_PTR id, DWORD) { KillTimer(h, id); step(pending); });
}

static void click(const wchar_t *name, const wchar_t *text) {
    wchar_t script[512];
    swprintf(script, 512, L"var a = document.createElement('a'); a.download = '%ls';"
             L"a.href = URL.createObjectURL(new Blob(['%ls'], {type: 'text/plain'}));"
             L"document.body.appendChild(a); a.click(); 'clicked'", name, text);
    webview->ExecuteScript(script, nullptr);
}

static void watch(ICoreWebView2DownloadOperation *op) {
    EventRegistrationToken t;
    op->add_BytesReceivedChanged(new Handler<ICoreWebView2BytesReceivedChangedEventHandler, ICoreWebView2DownloadOperation *, IUnknown *>(
        [](ICoreWebView2DownloadOperation *op, IUnknown *) {
            static bool once;
            INT64 got = 0; op->get_BytesReceived(&got);
            if (!once && got > 0) { once = true; log("BytesReceivedChanged %lld", got); }
            return S_OK; }), &t);
    op->add_StateChanged(new Handler<ICoreWebView2StateChangedEventHandler, ICoreWebView2DownloadOperation *, IUnknown *>(
        [](ICoreWebView2DownloadOperation *op, IUnknown *) {
            COREWEBVIEW2_DOWNLOAD_STATE state; op->get_State(&state);
            COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON reason; op->get_InterruptReason(&reason);
            LPWSTR path = nullptr; op->get_ResultFilePath(&path);
            INT64 total = 0; op->get_TotalBytesToReceive(&total);
            log("StateChanged state=%d reason=%d total=%lld", state, reason, total);
            if (state == COREWEBVIEW2_DOWNLOAD_STATE_COMPLETED) {
                bool custom = path && custom_path == path, deflt = path && default_path == path;
                log("completed custom=%d default=%d content=%s", custom, deflt, read_file(path).c_str());
                if (deflt) DeleteFileW(path);
                later(300, mode + 1);
            }
            CoTaskMemFree(path);
            return S_OK; }), &t);
}

static void step(int n) {
    log("--- step %d", n);
    switch (n) {
    case 1: mode = 1; click(L"first.txt", L"hello download"); break;
    case 2: mode = 2; click(L"second.txt", L"never"); later(3000, 3); break;
    case 3: {
        WIN32_FIND_DATAW fd; wchar_t pattern[MAX_PATH];
        wchar_t *dir = nullptr; SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, &dir);
        swprintf(pattern, MAX_PATH, L"%ls\\second*.txt", dir);
        HANDLE h = FindFirstFileW(pattern, &fd);
        log("cancelled file exists=%d", h != INVALID_HANDLE_VALUE);
        if (h != INVALID_HANDLE_VALUE) FindClose(h);
        CoTaskMemFree(dir);
        mode = 3; click(L"third.txt", L"deferred"); break; }
    case 4: log("done"); PostMessageW(hwnd, WM_CLOSE, 0, 0); break;
    }
}

static HRESULT on_download(ICoreWebView2 *, ICoreWebView2DownloadStartingEventArgs *args) {
    ICoreWebView2DownloadOperation *op; args->get_DownloadOperation(&op);
    LPWSTR uri = nullptr, mime = nullptr, path = nullptr;
    op->get_Uri(&uri); op->get_MimeType(&mime); args->get_ResultFilePath(&path);
    COREWEBVIEW2_DOWNLOAD_STATE state; op->get_State(&state);
    log("DownloadStarting mode=%d blob=%d mime=%s state=%d path=%s", mode, wcsncmp(uri, L"blob:", 5) == 0,
        utf8(mime).c_str(), state, utf8(path).c_str());
    switch (mode) {
    case 1:
        args->put_ResultFilePath(custom_path.c_str());
        watch(op);
        break;
    case 2:
        args->put_Cancel(TRUE);
        break;
    case 3:
        default_path = path;
        watch(op);
        args->GetDeferral(&deferral);
        SetTimer(hwnd, 11, 500, [](HWND h, UINT, UINT_PTR id, DWORD) {
            KillTimer(h, id); log("deferral complete"); deferral->Complete(); deferral->Release(); });
        break;
    }
    CoTaskMemFree(uri); CoTaskMemFree(mime); CoTaskMemFree(path);
    op->Release();
    return S_OK;
}

static LRESULT CALLBACK wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, msg, wp, lp);
}

static void CALLBACK on_timeout(HWND, UINT, UINT_PTR, DWORD) { log("timeout"); PostQuitMessage(2); }

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    wchar_t exe[MAX_PATH]; GetModuleFileNameW(nullptr, exe, MAX_PATH);
    *wcsrchr(exe, L'\\') = 0;
    custom_path = std::wstring(exe) + L"\\dl\\custom.txt";
    DeleteFileW(custom_path.c_str());
    WNDCLASSW wc = {}; wc.lpfnWndProc = wndproc; wc.hInstance = inst; wc.lpszClassName = L"dltest";
    RegisterClassW(&wc);
    hwnd = CreateWindowW(L"dltest", L"wine-webview2 dltest", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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
                            HRESULT hr = webview->QueryInterface(IID_ICoreWebView2_4, (void **)&webview4);
                            log("ICoreWebView2_4 hr=%#lx", hr);
                            webview4->add_DownloadStarting(new Handler<ICoreWebView2DownloadStartingEventHandler, ICoreWebView2 *,
                                ICoreWebView2DownloadStartingEventArgs *>(on_download), &t);
                            webview->add_PermissionRequested(new Handler<ICoreWebView2PermissionRequestedEventHandler, ICoreWebView2 *,
                                ICoreWebView2PermissionRequestedEventArgs *>([](ICoreWebView2 *, ICoreWebView2PermissionRequestedEventArgs *args) {
                                    COREWEBVIEW2_PERMISSION_KIND kind; args->get_PermissionKind(&kind);
                                    log("PermissionRequested kind=%d", kind);
                                    args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
                                    return S_OK; }), &t);
                            static bool started;
                            webview->add_NavigationCompleted(new Handler<ICoreWebView2NavigationCompletedEventHandler, ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *>(
                                [](ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *) {
                                    if (!started) { started = true; later(300, 1); } return S_OK; }), &t);
                            {
                                ICoreWebView2_3 *w3; webview->QueryInterface(IID_ICoreWebView2_3, (void **)&w3);
                                wchar_t dir[MAX_PATH]; GetModuleFileNameW(nullptr, dir, MAX_PATH); *wcsrchr(dir, L'\\') = 0;
                                std::wstring page = std::wstring(dir) + L"\\dl\\index.html";
                                CreateDirectoryW((std::wstring(dir) + L"\\dl").c_str(), nullptr);
                                HANDLE f = CreateFileW(page.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
                                DWORD n; WriteFile(f, "<body>downloads</body>", 22, &n, nullptr); CloseHandle(f);
                                w3->SetVirtualHostNameToFolderMapping(L"app.test", (std::wstring(dir) + L"\\dl").c_str(),
                                    COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
                                w3->Release();
                                webview->Navigate(L"https://app.test/index.html");
                            }
                            return S_OK; }));
                return S_OK; }));
    SetTimer(nullptr, 0, 40000, on_timeout);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}
