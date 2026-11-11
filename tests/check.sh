#!/bin/bash
# Runs the test programs through tests/run-app.sh and checks their logs for the expected lines.
# Exit status is non-zero if any test fails. Used by "make check" and CI.
#
# Build the test programs first (each tests/apps/*/Makefile needs WV2_SDK, hello also WEBVIEW_SRC).
# Tests whose executable is missing are skipped. Environment: see tests/run-app.sh (WINE, WINEPREFIX, ...).
set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
LOGS=$ROOT/build/logs
failed=0 passed=0 skipped=0

# check NAME EXE TIMEOUT ALLOWED_RC PATTERN...
check() {
    local name=$1 exe=$2 timeout=$3 allowed=$4 rc pattern missing=()
    shift 4
    if [ ! -f "$exe" ]; then
        echo "SKIP $name (not built: $exe)"
        skipped=$((skipped + 1))
        return
    fi
    local log=$LOGS/$name.log
    WV2_LOG=$log timeout "$timeout" "$ROOT/tests/run-app.sh" "$exe" > "$LOGS/$name.run.txt" 2>&1
    rc=$?
    for pattern in "$@"; do
        grep -aqE -- "$pattern" "$log" 2>/dev/null || missing+=("$pattern")
    done
    if [[ " $allowed " != *" $rc "* ]] || [ ${#missing[@]} -gt 0 ]; then
        echo "FAIL $name (exit $rc, log $log)"
        for pattern in "${missing[@]}"; do echo "     missing: $pattern"; done
        grep -v "^[0-9a-f]\{4\}:" "$LOGS/$name.run.txt" | tail -n 5 | sed 's/^/     | /' 
        failed=$((failed + 1))
    else
        echo "PASS $name"
        passed=$((passed + 1))
    fi
}

mkdir -p "$LOGS"
A=$ROOT/tests/apps

check navtest "$A/navtest/navtest.exe" 90 "0" \
    'NavigationCompleted id=1 success=1' \
    'Title=Page B CanGoBack=1'

# 32-bit application: the x86 DLL with a 32-bit unix library (old mode) or the 64-bit one (WoW64).
check navtest-x86 "$A/navtest/x86/navtest.exe" 90 "0" \
    'NavigationCompleted id=1 success=1' \
    'Title=Page B CanGoBack=1'

check bgtest "$A/bgtest/bgtest.exe" 90 "0" \
    'step 2 probe hr=0 .*webview=undefined' \
    'cdp Browser.getVersion hr=0' \
    'cdp event Runtime.consoleAPICalled'

check popuptest "$A/popuptest/popuptest.exe" 120 "0" \
    'NewWindowRequested uri=about:blank#click' \
    'popup: opener\+href -> hr=0 "true about:blank#deferred"' \
    'opener got: hi from https://app.test/popup.html' \
    'WindowCloseRequested' \
    "prompt\('default prompt', 'text'\) -> hr=0 \"text\"" \
    '^done'

check menutest "$A/menutest/menutest.exe" 90 "0" \
    'context menu: .*=117; .*=26501;' \
    'script -> hr=0 "0-10"' \
    'context menu: none' \
    '^done'

check apitest "$A/apitest/apitest.exe" 90 "0" \
    'UserDataFolder set=1' \
    'ControllerOptions name=apitest private=1' \
    'Profile name=Default' \
    '-> "dark=true"' \
    'ZoomFactorChanged 1.50' \
    '-> "dpr=1.5"' \
    '^GotFocus' \
    '^done'

# hello runs until closed: the timeout ending it is expected.
check hello "$A/hello/hello-loader.exe" 40 "0 124" \
    'bind called with \["ping"\]'

check tauri "$ROOT/build/tauri/x86_64-pc-windows-msvc/release/tauri-hello.exe" 90 "0" \
    'tauri: report: greet -> Hello, Wine!' \
    'event pong -> \{"value":42\}' \
    'tauri: finish'

echo "passed $passed, failed $failed, skipped $skipped"
[ "$failed" -eq 0 ]
