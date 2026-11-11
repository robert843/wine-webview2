#!/bin/bash
# Builds WebView2APISample (MicrosoftEdge/WebView2Samples) on Linux: clang-cl + lld-link + xwin.
#   tests/apps/apisample/build.sh
# Env: REF (dir with WebView2Samples, xwin, wine), LLVM (LLVM >= 18 bin dir), WIN10=1 (USE_WEBVIEW2_WIN10)
set -euo pipefail
REF=${REF:-$(cd "$(dirname "$0")/../../.." && pwd)/../wine-webview2-ref}
LLVM=${LLVM:-/usr/lib/llvm-20/bin}
SRC=$REF/WebView2Samples/SampleApps/WebView2APISample
PKG=$REF/WebView2Samples/SampleApps/packages
WV2=$PKG/Microsoft.Web.WebView2.1.0.4365-prerelease/build/native
WIL=$PKG/Microsoft.Windows.ImplementationLibrary.1.0.220201.1/include
X=$REF/xwin
OUT=$(cd "$(dirname "$0")" && pwd)/out
WIDL=$REF/wine-build/tools/widl/widl
mkdir -p "$OUT/obj"

SYS="-imsvc $X/crt/include -imsvc $X/sdk/include/ucrt -imsvc $X/sdk/include/um -imsvc $X/sdk/include/shared \
     -imsvc $X/sdk/include/winrt -imsvc $X/sdk/include/cppwinrt"
DEFS="/DUNICODE /D_UNICODE /DWIN32 /DNDEBUG /D_WINDOWS"
[ "${WIN10:-0}" = 1 ] && DEFS="$DEFS /DUSE_WEBVIEW2_WIN10"

echo "== header letter case"
for d in "$X/sdk/include/cppwinrt" "$X/sdk/include/um" "$X/sdk/include/shared" "$X/sdk/include/winrt" "$X/crt/include"; do
    python3 -I "$(dirname "$0")/fix-case.py" "$d" "$SRC" "$WIL" "$WV2/include"
done
ln -sfn "$SRC/resource.h" "$OUT/Resource.h"

echo "== patches for clang (MSVC accepts non-copyable objects passed to ...)"
sed -i 's/self->clientCertificates\[i\]\.Subject,$/self->clientCertificates[i].Subject.get(),/' "$SRC/ClientCertificateSelectionDialog.cpp"
# The sample assumes a prerelease runtime (Experimental interfaces); on stable try_query returns NULL.
sed -i 's/^    CHECK_FAILURE(exp_env15->add_RestartRequested($/    if (exp_env15) CHECK_FAILURE(exp_env15->add_RestartRequested(/' "$SRC/AppWindow.cpp"

echo "== widl HostObjectSample.idl"
# The type library (-t) crashes widl on this IDL; skipped (only the host object scenario needs it).
$WIDL -I"$REF/wine/include" -I"$REF/wine-build/include" -m64 \
      -h -H "$OUT/HostObjectSample_h.h" -u -U "$OUT/HostObjectSample_i.c" "$SRC/HostObjectSample.idl"

echo "== wrc (llvm-rc cannot parse this .rc)"
$REF/wine-build/tools/wrc/wrc --nostdinc -I"$SRC" -I"$REF/wine/include" -I"$REF/wine-build/include" \
      -o "$OUT/obj/WebView2APISample.res" "$SRC/WebView2APISample.rc"

echo "== clang-cl ($(ls "$SRC"/*.cpp | wc -l) files)"
compile() {
    local f=$1 o
    o="$OUT/obj/$(basename "${f%.*}").obj"
    [ "$o" -nt "$f" ] && return 0
    # TextInputDialog.h declares `friend DlgProcStatic`, so every file would export its own "static"
    # copy under the same name (MSVC tolerates it, lld does not); make the name unique per file.
    $LLVM/clang-cl /nologo /c /std:c++20 /Zc:char8_t- /MT /EHsc /O1 /Zc:__cplusplus /utf-8 -Wno-everything $DEFS $SYS \
        /D"DlgProcStatic=DlgProcStatic_$(basename "${f%.*}")" \
        /I "$SRC" /I "$OUT" /I "$WV2/include" /I "$WIL" /Fo"$o" "$f" || { echo "ERROR: $f"; return 1; }
}
export -f compile; export LLVM SYS DEFS SRC OUT WV2 WIL
ls "$SRC"/*.cpp "$OUT/HostObjectSample_i.c" | xargs -P "$(nproc)" -I{} bash -c 'compile {}'

echo "== lld-link"
$LLVM/lld-link /nologo /out:"$OUT/WebView2APISample.exe" /subsystem:windows /machine:x64 \
    /libpath:"$X/crt/lib/x86_64" /libpath:"$X/sdk/lib/um/x86_64" /libpath:"$X/sdk/lib/ucrt/x86_64" \
    "$OUT"/obj/*.obj "$OUT/obj/WebView2APISample.res" "$WV2/x64/WebView2Loader.dll.lib" \
    kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib shlwapi.lib ole32.lib \
    oleaut32.lib uuid.lib urlmon.lib gdiplus.lib comctl32.lib version.lib psapi.lib dcomp.lib d2d1.lib windowsapp.lib
cp "$WV2/x64/WebView2Loader.dll" "$OUT/"
cp -r "$SRC/assets" "$OUT/" 2>/dev/null || true
ls -la "$OUT/WebView2APISample.exe"
