#!/bin/bash
# Uruchamia aplikację Windows z naszym runtime'em WebView2 (zainstalowanym jak prawdziwy: pliki + rejestr).
#
#   tests/run-app.sh <program.exe> [argumenty…]
#
# Zmienne:
#   WINE         — binarka Wine ≥ 11.2 (domyślnie drzewo budowania z wine-webview2-ref)
#   WINEPREFIX   — prefiks testowy (domyślnie ~/.local/share/wine-webview2/prefix)
#   WINEDEBUG    — domyślnie +webview2 (TRACE i FIXME naszego kanału)
#   WV2_LOG      — plik logu (domyślnie build/logs/<program>.log)
#   WINE_WEBVIEW2_HELPER — natywny pomocnik (domyślnie build/runtime/wine-webview2-helper)
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
WINE=${WINE:-$ROOT/../wine-webview2-ref/wine-build/wine}
export WINEPREFIX=${WINEPREFIX:-$HOME/.local/share/wine-webview2/prefix}
export WINEDEBUG=${WINEDEBUG:-+webview2}

exe=$1; shift
name=$(basename "$exe" .exe)
log=${WV2_LOG:-$ROOT/build/logs/$name.log}
mkdir -p "$(dirname "$log")" "$WINEPREFIX"

if [ ! -f "$WINEPREFIX/system.reg" ]; then
    echo "== tworzę prefiks $WINEPREFIX"
    WINEDEBUG=-all "$WINE" wineboot -i >/dev/null 2>&1
fi

# Instalacja runtime'u tak jak prawdziwy (tryb „installed”):
#   C:\Program Files (x86)\Microsoft\EdgeWebView\Application\<wersja>\EBWebView\{x64,x86}\
#   + HKLM\SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\ClientState\{F3017226-…}\EBWebView = <katalog wersji>
ver=$(strings -el "$ROOT/build/x86_64/EmbeddedBrowserWebView.dll" | grep -A1 '^ProductVersion$' | tail -1)
rtwin="C:\\Program Files (x86)\\Microsoft\\EdgeWebView\\Application\\$ver"
rt="$WINEPREFIX/drive_c/Program Files (x86)/Microsoft/EdgeWebView/Application/$ver/EBWebView"
mkdir -p "$rt/x64" "$rt/x86"
cp "$ROOT/build/x86_64/EmbeddedBrowserWebView.dll" "$ROOT/build/x86_64/embeddedbrowserwebview.so" "$rt/x64/"
[ -f "$ROOT/build/i386/EmbeddedBrowserWebView.dll" ] && cp "$ROOT/build/i386/EmbeddedBrowserWebView.dll" "$rt/x86/"
WINEDEBUG=-all "$WINE" reg add 'HKLM\SOFTWARE\Microsoft\EdgeUpdate\ClientState\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}' \
    /v EBWebView /t REG_SZ /d "$rtwin" /f /reg:32 >/dev/null

# Pomocnik z CEF (docelowo /usr/lib/wine-webview2/; tu katalog z make runtime).
export WINE_WEBVIEW2_HELPER=${WINE_WEBVIEW2_HELPER:-$ROOT/build/runtime/wine-webview2-helper}

echo "== $exe  (log: $log)"
cd "$(dirname "$exe")"
"$WINE" "$(basename "$exe")" "$@" 2>&1 | tee "$log"
