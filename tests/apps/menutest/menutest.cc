// Test: default context menu (native popup menu built from CEF's model), choosing an item,
// AreDefaultContextMenusEnabled = FALSE.
#include <windows.h>
#include <cstdio>
#include <cwchar>
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

static void right_click() {
    HWND child = FindWindowExW(hwnd, nullptr, nullptr, nullptr);
    PostMessageW(child, WM_MOUSEMOVE, 0, MAKELPARAM(60, 30));
    PostMessageW(child, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(60, 30));
    PostMessageW(child, WM_RBUTTONUP, 0, MAKELPARAM(60, 30));
}

// Finds the popup menu, logs its items and picks command `choose_id` (0 = none) with the keyboard.
// By command id, not label: CEF localises the labels.
static UINT choose_id;
static DWORD WINAPI menu_driver(void *) {
    for (int i = 0; i < 50; i++) {
        Sleep(100);
        HWND menu_wnd = FindWindowW(L"#32768", nullptr);
        if (!menu_wnd || !IsWindowVisible(menu_wnd)) continue;
        HMENU menu = (HMENU)SendMessageW(menu_wnd, MN_GETHMENU, 0, 0);
        int count = GetMenuItemCount(menu), target = -1, position = 0;
        std::wstring items;
        for (int j = 0; j < count; j++) {
            wchar_t text[128] = L"";
            MENUITEMINFOW info = {sizeof(info), MIIM_FTYPE | MIIM_STATE};
            GetMenuItemInfoW(menu, j, TRUE, &info);
            if (info.fType & MFT_SEPARATOR) { items += L"| "; continue; }
            GetMenuStringW(menu, j, text, 128, MF_BYPOSITION);
            wchar_t id[16]; swprintf(id, 16, L"=%u; ", GetMenuItemID(menu, j));
            items += text; items += id;
            if (choose_id && GetMenuItemID(menu, j) == choose_id) target = position;
            position++;
        }
        char utf8[2048];
        WideCharToMultiByte(CP_UTF8, 0, items.c_str(), -1, utf8, sizeof(utf8), nullptr, nullptr);
        log("context menu: %s", utf8);
        Sleep(200);
        if (target < 0) { PostMessageW(menu_wnd, WM_KEYDOWN, VK_ESCAPE, 0); return 0; }
        for (int k = 0; k <= target; k++) PostMessageW(menu_wnd, WM_KEYDOWN, VK_DOWN, 0);
        PostMessageW(menu_wnd, WM_KEYDOWN, VK_RETURN, 0);
        log("chose command %u", choose_id);
        return 0;
    }
    log("context menu: none");
    return 0;
}

static void start_driver(UINT choose) {
    choose_id = choose;
    CloseHandle(CreateThread(nullptr, 0, menu_driver, nullptr, 0, nullptr));
}

static void step(int n) {
    log("--- step %d", n);
    switch (n) {
    case 1: // collapsed selection in the field, then right-click it and choose "Select all"
        run(L"var t = document.getElementById('t'); t.focus(); t.setSelectionRange(0, 0); 'ready'", 0);
        start_driver(117 /* MENU_ID_SELECT_ALL */);
        right_click();
        later(3000, 2); break;
    case 2: run(L"var t = document.getElementById('t'); t.selectionStart + '-' + t.selectionEnd", 3); break;
    case 3: { // menus disabled: nothing may appear
        ICoreWebView2Settings *s; webview->get_Settings(&s);
        s->put_AreDefaultContextMenusEnabled(FALSE); s->Release();
        start_driver(0);
        right_click();
        later(6000, 4); break; }
    case 4: log("done"); PostMessageW(hwnd, WM_CLOSE, 0, 0); break;
    }
}

static LRESULT CALLBACK wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, msg, wp, lp);
}

static void CALLBACK on_timeout(HWND, UINT, UINT_PTR, DWORD) { log("timeout"); PostQuitMessage(2); }

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    WNDCLASSW wc = {}; wc.lpfnWndProc = wndproc; wc.hInstance = inst; wc.lpszClassName = L"menutest";
    RegisterClassW(&wc);
    hwnd = CreateWindowW(L"menutest", L"wine-webview2 menutest", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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
                            webview->NavigateToString(L"<body style='margin:0'><input id=t value='hello menu' "
                                                      L"style='position:absolute;left:10px;top:10px;width:300px;height:40px;font:20px sans-serif'>");
                            return S_OK; }));
                return S_OK; }));
    SetTimer(nullptr, 0, 40000, on_timeout);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}
