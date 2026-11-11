// Test: NewWindowRequested (Handled, odroczenie + NewWindow, okno domyślne), WindowCloseRequested,
// ScriptDialogOpening (AreDefaultScriptDialogsEnabled = FALSE) i dialogi domyślne.
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

static const GUID iid_newwindow_args2 = {0xbbc7baed, 0x74c6, 0x4c92, {0xb6, 0x3a, 0x7f, 0x5a, 0xea, 0xe0, 0x3d, 0xe3}};
static ICoreWebView2Environment *env;
static ICoreWebView2 *webview, *popup_webview;
static ICoreWebView2Controller *controller, *popup_controller;
static HWND hwnd;
static enum { POPUP_HANDLED, POPUP_DEFER, POPUP_DEFAULT } popup_mode;
static enum { DLG_ACCEPT, DLG_PROMPT, DLG_DEFER } dialog_mode;
static ICoreWebView2Deferral *dialog_deferral;
static ICoreWebView2ScriptDialogOpeningEventArgs *dialog_args;

static void log(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf("\n"); fflush(stdout);
}

static void step(int n);

static void run(const wchar_t *script, int next) {
    webview->ExecuteScript(script, new Handler<ICoreWebView2ExecuteScriptCompletedHandler, HRESULT, LPCWSTR>(
        [script, next](HRESULT hr, LPCWSTR json) {
            log("script %.60ls -> hr=%#lx %ls", script, hr, json ? json : L"(null)");
            if (next) step(next);
            return S_OK; }));
}

static void later(int ms, int n) {
    static int pending;
    pending = n;
    SetTimer(hwnd, 10, ms, [](HWND h, UINT, UINT_PTR id, DWORD) { KillTimer(h, id); step(pending); });
}

// Domyślny dialog (MessageBox / prompt w DLL): znajdź i kliknij OK.
static DWORD WINAPI clicker(void *) {
    for (int i = 0; i < 100; i++) {
        Sleep(100);
        HWND dlg = FindWindowW(L"#32770", nullptr);
        if (!dlg) continue;
        wchar_t title[256]; GetWindowTextW(dlg, title, 256);
        log("default dialog found: \"%ls\"", title);
        Sleep(200);
        PostMessageW(dlg, WM_COMMAND, IDOK, 0);
        return 0;
    }
    log("default dialog NOT found");
    return 0;
}

static void step(int n) {
    log("--- step %d", n);
    switch (n) {
    case 1: popup_mode = POPUP_HANDLED;
        run(L"String(window.open('about:blank#handled', 'win1', 'width=320,height=240'))", 0);
        later(1000, 12); break;
    case 12: { // kliknięcie (gest użytkownika) w przycisk z window.open
        HWND child = FindWindowExW(hwnd, nullptr, nullptr, nullptr);
        log("click child=%p", child);
        PostMessageW(child, WM_MOUSEMOVE, 0, MAKELPARAM(50, 50));
        PostMessageW(child, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(50, 50));
        PostMessageW(child, WM_LBUTTONUP, 0, MAKELPARAM(50, 50));
        later(1000, 13); break; }
    case 13: run(L"String(window.clicked)", 2); break;
    case 2: popup_mode = POPUP_DEFER;
        run(L"String(window.open('about:blank#deferred', 'win2'))", 0); later(3000, 3); break;
    case 3: run(L"window.close(); 'closed?'", 0); later(500, 4); break;
    case 4: {
        ICoreWebView2Settings *s; webview->get_Settings(&s);
        s->put_AreDefaultScriptDialogsEnabled(FALSE); s->Release();
        dialog_mode = DLG_ACCEPT; run(L"confirm('pytanie?')", 5); break; }
    case 5: dialog_mode = DLG_PROMPT; run(L"prompt('imię?', 'domyślne')", 6); break;
    case 6: dialog_mode = DLG_DEFER; run(L"confirm('odroczone?')", 7);
        SetTimer(hwnd, 11, 500, [](HWND h, UINT, UINT_PTR id, DWORD) {
            KillTimer(h, id);
            log("completing dialog deferral");
            dialog_args->Accept(); dialog_deferral->Complete();
            dialog_deferral->Release(); dialog_args->Release(); });
        break;
    case 7: {
        ICoreWebView2Settings *s; webview->get_Settings(&s);
        s->put_AreDefaultScriptDialogsEnabled(TRUE); s->Release();
        CloseHandle(CreateThread(nullptr, 0, clicker, nullptr, 0, nullptr));
        run(L"prompt('domyślny prompt', 'tekst')", 8); break; }
    case 8:
        CloseHandle(CreateThread(nullptr, 0, clicker, nullptr, 0, nullptr));
        run(L"confirm('domyślny confirm')", 9); break;
    case 9: popup_mode = POPUP_DEFAULT;
        run(L"window.w3 = window.open('about:blank#default', 'win3', 'width=300,height=200'); String(!!w3)", 0);
        later(2000, 10); break;
    case 10: run(L"w3.close(); 'ok'", 0); later(1000, 11); break;
    case 11: log("done"); PostMessageW(hwnd, WM_CLOSE, 0, 0); break;
    }
}

static void register_events() {
    EventRegistrationToken t;
    webview->add_NewWindowRequested(new Handler<ICoreWebView2NewWindowRequestedEventHandler, ICoreWebView2 *, ICoreWebView2NewWindowRequestedEventArgs *>(
        [](ICoreWebView2 *, ICoreWebView2NewWindowRequestedEventArgs *a) {
            LPWSTR uri = nullptr, name = nullptr; BOOL user; a->get_Uri(&uri); a->get_IsUserInitiated(&user);
            ICoreWebView2NewWindowRequestedEventArgs2 *a2;
            if (SUCCEEDED(a->QueryInterface(iid_newwindow_args2, (void **)&a2))) { a2->get_Name(&name); a2->Release(); }
            ICoreWebView2WindowFeatures *f; BOOL has_size = FALSE; UINT32 w = 0, h = 0;
            if (SUCCEEDED(a->get_WindowFeatures(&f)) && f) { f->get_HasSize(&has_size); f->get_Width(&w); f->get_Height(&h); f->Release(); }
            log("NewWindowRequested uri=%ls name=%ls user=%d size=%d %ux%u mode=%d", uri, name ? name : L"?", user, has_size, w, h, popup_mode);
            CoTaskMemFree(uri); CoTaskMemFree(name);
            if (popup_mode == POPUP_HANDLED) a->put_Handled(TRUE);
            else if (popup_mode == POPUP_DEFER) {
                ICoreWebView2Deferral *d; a->GetDeferral(&d); a->AddRef();
                env->CreateCoreWebView2Controller(hwnd, new Handler<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, HRESULT, ICoreWebView2Controller *>(
                    [a, d](HRESULT hr, ICoreWebView2Controller *c) {
                        log("popup controller hr=%#lx", hr);
                        popup_controller = c; c->AddRef();
                        c->get_CoreWebView2(&popup_webview);
                        RECT r = {300, 0, 600, 200}; c->put_Bounds(r);
                        EventRegistrationToken t2;
                        popup_webview->add_NavigationCompleted(new Handler<ICoreWebView2NavigationCompletedEventHandler, ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *>(
                            [](ICoreWebView2 *s, ICoreWebView2NavigationCompletedEventArgs *) {
                                LPWSTR src; s->get_Source(&src); log("popup webview NavigationCompleted source=%ls", src); CoTaskMemFree(src);
                                return S_OK; }), &t2);
                        log("put_NewWindow hr=%#lx", a->put_NewWindow(popup_webview));
                        d->Complete(); d->Release(); a->Release();
                        return S_OK; }));
            }
            return S_OK; }), &t);
    webview->add_WindowCloseRequested(new Handler<ICoreWebView2WindowCloseRequestedEventHandler, ICoreWebView2 *, IUnknown *>(
        [](ICoreWebView2 *, IUnknown *) { log("WindowCloseRequested"); return S_OK; }), &t);
    webview->add_ScriptDialogOpening(new Handler<ICoreWebView2ScriptDialogOpeningEventHandler, ICoreWebView2 *, ICoreWebView2ScriptDialogOpeningEventArgs *>(
        [](ICoreWebView2 *, ICoreWebView2ScriptDialogOpeningEventArgs *a) {
            COREWEBVIEW2_SCRIPT_DIALOG_KIND kind; LPWSTR msg, def, uri;
            a->get_Kind(&kind); a->get_Message(&msg); a->get_DefaultText(&def); a->get_Uri(&uri);
            log("ScriptDialogOpening kind=%d message=%ls default=%ls uri=%.40ls", kind, msg, def, uri);
            CoTaskMemFree(msg); CoTaskMemFree(def); CoTaskMemFree(uri);
            if (dialog_mode == DLG_ACCEPT) a->Accept();
            else if (dialog_mode == DLG_PROMPT) { a->put_ResultText(L"Zażółć"); a->Accept(); }
            else { a->GetDeferral(&dialog_deferral); dialog_args = a; a->AddRef(); }
            return S_OK; }), &t);
}

static LRESULT CALLBACK wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSW wc = {}; wc.lpfnWndProc = wndproc; wc.hInstance = inst; wc.lpszClassName = L"popuptest";
    RegisterClassW(&wc);
    hwnd = CreateWindowW(L"popuptest", L"wine-webview2 popuptest", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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
                            RECT r = {0, 0, 300, 360}; controller->put_Bounds(r);
                            register_events();
                            EventRegistrationToken t;
                            static bool started;
                            webview->add_NavigationCompleted(new Handler<ICoreWebView2NavigationCompletedEventHandler, ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *>(
                                [](ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *) {
                                    if (!started) { started = true; step(1); } return S_OK; }), &t);
                            webview->NavigateToString(L"<title>popuptest</title><body style='font:20px sans-serif;margin:0'><button style='width:300px;height:200px' onclick=\"window.clicked=String(window.open('about:blank#click','winc'))\">open</button></body>");
                            return S_OK; }));
                return S_OK; }));
    SetTimer(nullptr, 0, 40000, [](HWND, UINT, UINT_PTR, DWORD) { log("timeout"); PostQuitMessage(2); });
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}
