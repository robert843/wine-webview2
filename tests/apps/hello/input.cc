// Aplikacja testowa E3: wejście (mysz, klawiatura, kółko, fokus) — zdarzenia wracają mostem JS na stderr.
#include "webview/webview.h"
#include <cstdio>

static const char *page = R"HTML(<!doctype html><meta charset=utf-8>
<style>body{font:24px sans-serif;margin:16px} input,button{font:24px sans-serif}</style>
<input id=t placeholder="pisz tutaj"> <button id=b>kliknij</button>
<div id=state>start</div>
<div style="height:3000px;background:linear-gradient(#fff,#36c)">przewiń w dół</div>
<script>
let clicks = 0;
const say = s => { report(s); document.getElementById('state').textContent = s; };
b.onclick = () => say('click ' + (++clicks));
t.oninput = () => say('input ' + t.value);
document.addEventListener('keydown', e => report('keydown ' + e.key + ' ' + e.code));
let last = 0;
window.addEventListener('scroll', () => { if (Math.abs(scrollY - last) > 50) { last = scrollY; report('scroll ' + Math.round(scrollY)); } });
t.onfocus = () => report('focus input');
</script>)HTML";

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    webview::webview w(false, nullptr);
    w.set_title("wine-webview2 input");
    w.set_size(800, 600, WEBVIEW_HINT_NONE);
    w.bind("report", [](const std::string &req) -> std::string {
        std::fprintf(stderr, "report: %s\n", req.c_str());
        std::fflush(stderr);
        return "null";
    });
    w.set_html(page);
    w.run();
    return 0;
}
