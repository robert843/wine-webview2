# wine-webview2

An open implementation of **Microsoft Edge WebView2** for **Wine and Proton**.

Windows applications built on WebView2 - game launchers, Tauri and Wails apps, tools with embedded
web UIs - run under Wine without installing Microsoft's runtime. The web content is rendered by a
native Linux build of Chromium ([CEF](https://bitbucket.org/chromiumembedded/cef)), the same way
[Wine Gecko](https://gitlab.winehq.org/wine/wine/-/wikis/Gecko) stands in for Internet Explorer and
[DXVK](https://github.com/doitsujin/dxvk) for Direct3D.

> **Status: 0.1.0 - preview.** It works with real applications, but many WebView2 APIs are still
> stubs. Expect rough edges and please report what breaks.

## What works

Tested with:

- **FINAL FANTASY XIV** official launcher - Steam version on Proton Experimental and the standalone
  installer: license, account creation, login, launcher home page. No Microsoft runtime, no
  `WebView2RuntimeInvalid` workaround.
- **Tauri 2** applications (custom protocol, `invoke`, events) and **Wails v3** applications
  ([LeviLauncher](https://github.com/LiteLDev/LeviLauncher)).
- [webview/webview](https://github.com/webview/webview) and Microsoft's **WebView2APISample** (start page).

Implemented: environment and controller creation, navigation and its events, `ExecuteScript` /
`ExecuteScriptWithResult`, `AddScriptToExecuteOnDocumentCreated`, the `chrome.webview` bridge
(`postMessage`, `WebMessageReceived`), `WebResourceRequested` and virtual host mappings, settings
(user agent, dev tools, script dialogs, …), transparent backgrounds, DevTools Protocol
(`CallDevToolsProtocolMethod`, event receivers), popups (`NewWindowRequested` with `window.opener`),
`WindowCloseRequested`, JavaScript dialogs, mouse/keyboard/cursor input, `<select>` popups.

Copy and paste works in both directions with Windows and Linux applications (CEF and Wine share
the system clipboard).

Not yet: IME, drag and drop, file chooser, DPI scaling, context menus,
downloads, profiles, `AddHostObjectToScript`, composition mode (`CompositionController`).

## Requirements

- **Wine 11.2 or newer**, or **Proton Experimental**. The runtime loads its Unix library with an API
  that older Wine (including the stable Proton 11.0) does not have.
- Linux x86_64. 64-bit and 32-bit Windows applications (32-bit works in both Wine modes: the new
  WoW64 mode and the classic one with a 32-bit Unix side).
- Wine built with GnuTLS (all distribution packages are), otherwise HTTPS does not work at all.

## Installation

Download `wine-webview2-<version>.tar.xz` from the releases page and unpack it, then:

```sh
./wine-webview2 install              # browser helper (CEF, ~370 MB) into ~/.local/share/wine-webview2/<version>
./wine-webview2 prefix ~/.wine       # runtime into a Wine prefix
./wine-webview2 steam 39210          # …or into the Proton prefix of a Steam game (run the game once first)
./wine-webview2 status ~/.wine
./wine-webview2 uninstall ~/.wine    # back to Microsoft's runtime if it was installed
./wine-webview2 steam-uninstall 39210
```

For Steam games, select **Proton Experimental** in the game's compatibility settings.

The helper is installed once per user and shared by all prefixes; each prefix gets a few small files in
`C:\Program Files (x86)\Microsoft\EdgeWebView\Application\156.0.9999.0\` plus the registry values the
WebView2 loader and applications look for. Microsoft's runtime, if present, is left untouched.

## How it works

```
 Windows application (in Wine)                 native Linux
 WebView2Loader / statically linked loader
   └─ EmbeddedBrowserWebView.dll  ──unixlib──▶  socketpair + memfd  ──▶  wine-webview2-helper (CEF)
      thin host: window, painting, input           versioned protocol        WebView2 API logic (Rust),
      generic COM proxy generated from IDL                                    off-screen Chromium
```

The application sees the regular `ICoreWebView2*` COM interfaces. A generator turns Microsoft's IDL
into a generic proxy, so calls are forwarded to the helper, where the WebView2 behaviour is
implemented in Rust on top of CEF. Frames come back through shared memory and are painted into the
application's window. Like the real runtime, the browser lives in a separate process.

## Building from source

You need a built Wine source tree (≥ 11.2; for `widl`, `wrc`, `winegcc` and the import libraries),
mingw-w64 (x86_64 and i686), Rust, and the usual Chromium runtime libraries. CEF is downloaded by
the `cef` crate during the build.

```sh
make REF=/path/to/dir-with-wine-and-wine-build    # or WINE_SRC=… WINE_BUILD=… CEF_PATH=…
make dist                                          # build/dist/wine-webview2-<version>.tar.xz
./wine-webview2 install                            # from the source tree works too
```

Tests (`tests/run-app.sh`, `tests/apps/*`) run small WebView2 programs in a test prefix on a hidden
Xvfb display.

## Debugging

- `WINEDEBUG=+webview2` - trace calls in the DLL; JavaScript console output appears as `helper: console[…]`.
- `WINE_WEBVIEW2_DEVTOOLS_PORT=9229` - remote DevTools; open `chrome://inspect` in Chrome/Chromium
  and add `localhost:9229`. Works for Steam games too (add it to the launch options).
- `WINE_WEBVIEW2_HELPER=/path/to/wine-webview2-helper` - use a specific helper.

## About this project

This project was written largely by Claude (Anthropic's AI assistant) under my direction. I made the
design decisions, ran and debugged it against real applications (Tauri, LeviLauncher, the FFXIV
launcher on Steam/Proton), and decided what goes in - but I haven't hand-reviewed every line. Bug
reports and reviews are very welcome.

## License

wine-webview2 is licensed under the **GNU LGPL, version 2.1 or later** (see `LICENSE`), like Wine.

- `include/webview2.idl` comes from Microsoft's WebView2 SDK and keeps its own BSD license
  (`include/webview2.idl.LICENSE`).
- Release packages contain CEF/Chromium binaries: CEF is BSD-licensed (`third_party/CEF-LICENSE.txt`),
  Chromium's third-party notices are in `helper/CHROMIUM-CREDITS.html`.
- `patches/wine/` contains Wine patches (LGPL) found while testing Microsoft Teams; they are not
  needed to use wine-webview2.

This project is not affiliated with Microsoft. WebView2 and Microsoft Edge are trademarks of Microsoft.
