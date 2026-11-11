#!/bin/bash
# Ranks WebView2 method calls in logs (WINEDEBUG=+webview2): call count and number of apps.
#   tests/rank.sh build/logs/*.log
set -euo pipefail
for f in "$@"; do
    app=$(basename "$f" .log)
    grep -oE '(fixme|trace):webview2:[A-Za-z0-9_]+' "$f" | sed -E 's/^(fixme|trace):webview2://' \
        | grep -vE '^(stub_(AddRef|Release|create)|DllMain|invoke_.*|.*_cb)$' | sed "s/^/$app /"
done | awk '{calls[$2]++; if (!seen[$2 SUBSEP $1]++) apps[$2]++}
            END {for (m in calls) printf "%-6d %-6d %s\n", apps[m], calls[m], m}' \
    | sort -k1,1nr -k2,2nr | { printf "%-6s %-6s %s\n" apps calls method; cat; }
