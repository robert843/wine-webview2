// Test: default context menu (native popup menu built from CEF's model), choosing an item,
// ContextMenuRequested (custom items, submenu, CustomItemSelected, handled by the app),
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

static ICoreWebView2Environment *env;
static int menu_mode = 1;   // 1 = change the menu, 2 = handle it, 0 = leave it

static HRESULT on_menu_requested(ICoreWebView2 *, ICoreWebView2ContextMenuRequestedEventArgs *args) {
    if (!menu_mode) return S_OK;
    ICoreWebView2ContextMenuTarget *target; args->get_ContextMenuTarget(&target);
    COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND kind; BOOL editable;
    target->get_Kind(&kind); target->get_IsEditable(&editable); target->Release();
    ICoreWebView2ContextMenuItemCollection *items; args->get_MenuItems(&items);
    UINT32 count; items->get_Count(&count);
    std::string names;
    INT32 select_all = -1;
    for (UINT32 i = 0; i < count; i++) {
        ICoreWebView2ContextMenuItem *item; items->GetValueAtIndex(i, &item);
        LPWSTR name; item->get_Name(&name);
        char utf8[64]; WideCharToMultiByte(CP_UTF8, 0, name, -1, utf8, sizeof(utf8), nullptr, nullptr);
        names += utf8; names += ",";
        if (!wcscmp(name, L"selectAll")) { item->get_CommandId(&select_all); if (menu_mode == 1) { items->RemoveValueAtIndex(i); i--; count--; } }
        CoTaskMemFree(name); item->Release();
    }
    log("ContextMenuRequested kind=%d editable=%d items=%s", kind, editable, names.c_str());
    if (menu_mode == 2) {
        args->put_Handled(TRUE);
        args->put_SelectedCommandId(select_all);
    } else {
        ICoreWebView2Environment9 *e9; env->QueryInterface(IID_ICoreWebView2Environment9, (void **)&e9);
        ICoreWebView2ContextMenuItem *custom, *sub, *child;
        e9->CreateContextMenuItem(L"Custom item", nullptr, COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_COMMAND, &custom);
        e9->CreateContextMenuItem(L"More", nullptr, COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_SUBMENU, &sub);
        e9->CreateContextMenuItem(L"Nested", nullptr, COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_COMMAND, &child);
        ICoreWebView2ContextMenuItemCollection *children; sub->get_Children(&children);
        children->InsertValueAtIndex(0, child); children->Release();
        EventRegistrationToken t;
        custom->add_CustomItemSelected(new Handler<ICoreWebView2CustomItemSelectedEventHandler, ICoreWebView2ContextMenuItem *, IUnknown *>(
            [](ICoreWebView2ContextMenuItem *item, IUnknown *) {
                LPWSTR label; item->get_Label(&label);
                log("CustomItemSelected %ls", label); CoTaskMemFree(label);
                return S_OK; }), &t);
        INT32 id; custom->get_CommandId(&id);
        log("custom item id=%d", id);
        items->InsertValueAtIndex(0, custom);
        items->InsertValueAtIndex(1, sub);
        custom->Release(); sub->Release(); child->Release(); e9->Release();
    }
    items->Release();
    return S_OK;
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
    case 3: { // ContextMenuRequested: remove "selectAll", add a custom item and a submenu, choose the custom item
        ICoreWebView2_11 *w11; webview->QueryInterface(IID_ICoreWebView2_11, (void **)&w11);
        EventRegistrationToken t;
        w11->add_ContextMenuRequested(new Handler<ICoreWebView2ContextMenuRequestedEventHandler, ICoreWebView2 *,
            ICoreWebView2ContextMenuRequestedEventArgs *>(on_menu_requested), &t);
        w11->Release();
        run(L"var t = document.getElementById('t'); t.focus(); t.setSelectionRange(0, 0); 'ready'", 0);
        start_driver(50000);
        right_click();
        later(3000, 4); break; }
    case 4: // handled by the app: select all without a menu
        menu_mode = 2;
        run(L"var t = document.getElementById('t'); t.focus(); t.setSelectionRange(0, 0); 'ready'", 0);
        start_driver(0);
        right_click();
        later(3000, 5); break;
    case 5: run(L"var t = document.getElementById('t'); 'handled ' + t.selectionStart + '-' + t.selectionEnd", 6); break;
    case 6: { // menus disabled: nothing may appear
        menu_mode = 0;
        ICoreWebView2Settings *s; webview->get_Settings(&s);
        s->put_AreDefaultContextMenusEnabled(FALSE); s->Release();
        start_driver(0);
        right_click();
        later(6000, 7); break; }
    case 7: log("done"); PostMessageW(hwnd, WM_CLOSE, 0, 0); break;
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
