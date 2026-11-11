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
# Browser data persists in "<exe>.WebView2" (permissions, cookies...): every run starts clean.
rm -rf "$A"/*/*.exe.WebView2 "$A"/*/x86/*.exe.WebView2

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
    'AcceleratorKeyPressed kind=0 key=0x71 released=0' \
    'AcceleratorKeyPressed kind=1 key=0x72 released=1' \
    '-> "keys=F3"' \
    'GetFavicon hr=0 png=1' \
    'StatusBarTextChanged https://example.test/hover' \
    'IsMuted=1' \
    '^IsMutedChanged' \
    '-> "dpr=3 width=213"' \
    '^done'

check dltest "$A/dltest/dltest.exe" 90 "0" \
    'DownloadStarting mode=1 blob=1 mime=text/plain state=0 path=C:.*\\first.txt' \
    'completed custom=1 default=0 content=hello download' \
    'PermissionRequested kind=7' \
    'cancelled file exists=0' \
    'completed custom=0 default=1 content=deferred' \
    '^done'

check filetest "$A/filetest/filetest.exe" 90 "0" \
    'script -> hr=0 "pick.txt:picked content"' \
    'cancelled' \
    'still alive' \
    '^done'

check hosttest "$A/hosttest/hosttest.exe" 90 "0" \
    'AddHostObjectToScript hr=0' \
    'result add=5;name=calc;renamed=renamed;chain=11;childname=child;echo=\[1,"two",true,null,2.5\];nameof=child;fail=boom;missing=Element not found: Missing;nothere=Element not found: nothere;sync.add=9;sync.name=renamed;sync.renamed=syncset;sync.child=child;sync.echo=\["a",1\];sync.fail=boom;tosync=2' \
    '^done'

# Browser data persists in the user data folder: the second run sees the first one's data.
check persisttest-1 "$A/persisttest/persisttest.exe" 90 "0" \
    'default: script -> hr=0 "n=1 cookie=false"' \
    'private: profile name=Default private=1' \
    'second: profile name=second private=0' \
    'second: script -> hr=0 "n=1 cookie=false"' '^done'
check persisttest-2 "$A/persisttest/persisttest.exe" 90 "0" \
    'default: script -> hr=0 "n=2 cookie=true"' \
    'private: script -> hr=0 "n=1 cookie=false"' \
    'second: script -> hr=0 "n=2 cookie=true"' '^done'

# hello runs until closed: the timeout ending it is expected.
check hello "$A/hello/hello-loader.exe" 40 "0 124" \
    'bind called with \["ping"\]'

check tauri "$ROOT/build/tauri/x86_64-pc-windows-msvc/release/tauri-hello.exe" 90 "0" \
    'tauri: report: greet -> Hello, Wine!' \
    'event pong -> \{"value":42\}' \
    'tauri: finish'

echo "passed $passed, failed $failed, skipped $skipped"
[ "$failed" -eq 0 ]
