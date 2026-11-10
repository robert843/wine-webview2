// Aplikacja testowa E1: minimalne użycie WebView2 przez webview/webview.
// Ćwiczy: środowisko, kontroler, Navigate/NavigateToString, AddScriptToExecuteOnDocumentCreated,
// ExecuteScript, WebMessageReceived (bind).
#include "webview/webview.h"
#include <cstdio>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    try {
        webview::webview w(true, nullptr);
        w.set_title("wine-webview2 hello");
        w.set_size(800, 600, WEBVIEW_HINT_NONE);
        w.init("window.__wv2_init = 1;");
        w.bind("hello", [&](const std::string &req) -> std::string {
            std::fprintf(stderr, "hello: bind called with %s\n", req.c_str());
            return "\"world\"";
        });
        w.set_html("<h1>Hello from wine-webview2</h1>"
                   "<script>hello('ping').then(r => document.body.append(r));</script>");
        w.eval("document.title = 'eval ok';");
        w.run();
    } catch (const webview::exception &e) {
        std::fprintf(stderr, "webview error: %s\n", e.what());
        return 1;
    }
    return 0;
}
