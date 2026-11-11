#!/bin/bash
# Runs a Windows app with our WebView2 runtime, installed like the real one (files + registry).
#
#   tests/run-app.sh <program.exe> [args...]
#
# Environment:
#   WINE          - Wine >= 11.2 binary (default: the build tree in wine-webview2-ref)
#   WINEPREFIX    - test prefix (default ~/.local/share/wine-webview2/prefix)
#   WINEDEBUG     - default +webview2 (TRACE and FIXME of our channel)
#   WV2_LOG       - log file (default build/logs/<program>.log)
#   WINE_WEBVIEW2_RUNTIME - helper directory (default build/runtime)
#   WINE_WEBVIEW2_HELPER  - override the helper binary itself
#   WV2_VISIBLE   - 1 = windows on the user's desktop; default is a hidden Xvfb display
#                   (WV2_DISPLAY, :99) so tests don't flash windows (screenshots: DISPLAY=:99 import -window ...)
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
REF=${REF:-$ROOT/../wine-webview2-ref}
WINE=${WINE:-$REF/wine-build/wine}
export WINEPREFIX=${WINEPREFIX:-$HOME/.local/share/wine-webview2/prefix}
export WINEDEBUG=${WINEDEBUG:-+webview2}

if [ "${WV2_VISIBLE:-0}" != 1 ]; then
    export DISPLAY=${WV2_DISPLAY:-:99}
    if ! xdpyinfo -display "$DISPLAY" >/dev/null 2>&1; then
        Xvfb "$DISPLAY" -screen 0 1600x1000x24 -nolisten tcp >/dev/null 2>&1 &
        for _ in $(seq 1 50); do xdpyinfo -display "$DISPLAY" >/dev/null 2>&1 && break; sleep 0.1; done
    fi
fi

exe=$1; shift
name=$(basename "$exe" .exe)
log=${WV2_LOG:-$ROOT/build/logs/$name.log}
mkdir -p "$(dirname "$log")" "$WINEPREFIX"

# Clean start: kill leftovers of previous runs in this prefix (e.g. debugger windows).
WINESERVER=${WINESERVER:-$(dirname "$WINE")/server/wineserver}
[ -x "$WINESERVER" ] || WINESERVER=wineserver
"$WINESERVER" -k >/dev/null 2>&1 || true
sleep 0.5

if [ ! -f "$WINEPREFIX/system.reg" ]; then
    echo "== creating prefix $WINEPREFIX"
    WINEDEBUG=-all "$WINE" wineboot -i >/dev/null 2>&1
fi

# Install the runtime with the same script users run (helper straight from build/runtime).
export WINE_WEBVIEW2_RUNTIME=${WINE_WEBVIEW2_RUNTIME:-$ROOT/build/runtime}
WINE="$WINE" "$ROOT/wine-webview2" prefix "$WINEPREFIX" >/dev/null

echo "== $exe  (log: $log)"
cd "$(dirname "$exe")"
"$WINE" "$(basename "$exe")" "$@" 2>&1 | tee "$log"
