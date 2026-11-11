#!/bin/bash
# Uruchamia aplikację Windows z naszym runtime'em WebView2 (zainstalowanym jak prawdziwy: pliki + rejestr).
#
#   tests/run-app.sh <program.exe> [argumenty…]
#
# Zmienne:
#   WINE         - binarka Wine ≥ 11.2 (domyślnie drzewo budowania z wine-webview2-ref)
#   WINEPREFIX   - prefiks testowy (domyślnie ~/.local/share/wine-webview2/prefix)
#   WINEDEBUG    - domyślnie +webview2 (TRACE i FIXME naszego kanału)
#   WV2_LOG      - plik logu (domyślnie build/logs/<program>.log)
#   WINE_WEBVIEW2_HELPER - natywny pomocnik (domyślnie build/runtime/wine-webview2-helper)
#   WV2_VISIBLE  - 1 = okna na pulpicie użytkownika; domyślnie niewidoczny ekran Xvfb (WV2_DISPLAY, :99),
#                  żeby testy nie migały oknami (zrzuty: DISPLAY=:99 import -window …)
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

# Czysty start: zamknij resztki poprzednich przebiegów w tym prefiksie (np. okna debuggera).
WINESERVER=${WINESERVER:-$(dirname "$WINE")/server/wineserver}
[ -x "$WINESERVER" ] || WINESERVER=wineserver
"$WINESERVER" -k >/dev/null 2>&1 || true
sleep 0.5

if [ ! -f "$WINEPREFIX/system.reg" ]; then
    echo "== tworzę prefiks $WINEPREFIX"
    WINEDEBUG=-all "$WINE" wineboot -i >/dev/null 2>&1
fi

# Instalacja runtime'u tak jak prawdziwy - tym samym skryptem co u użytkowników (pomocnik prosto z build/runtime).
export WINE_WEBVIEW2_RUNTIME=${WINE_WEBVIEW2_RUNTIME:-$ROOT/build/runtime}
WINE="$WINE" "$ROOT/wine-webview2" prefix "$WINEPREFIX" >/dev/null

echo "== $exe  (log: $log)"
cd "$(dirname "$exe")"
"$WINE" "$(basename "$exe")" "$@" 2>&1 | tee "$log"
