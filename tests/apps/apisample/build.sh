#!/bin/bash
# Buduje WebView2APISample (MicrosoftEdge/WebView2Samples) na Linuksie: clang-cl + lld-link + xwin.
#   tests/apps/apisample/build.sh
# Zmienne: REF (katalog z WebView2Samples, xwin, wine), LLVM (katalog bin LLVM >= 18), WIN10=1 (USE_WEBVIEW2_WIN10)
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

echo "== wielkość liter w nagłówkach"
for d in "$X/sdk/include/cppwinrt" "$X/sdk/include/um" "$X/sdk/include/shared" "$X/sdk/include/winrt" "$X/crt/include"; do
    python3 -I "$(dirname "$0")/fix-case.py" "$d" "$SRC" "$WIL" "$WV2/include"
done
ln -sfn "$SRC/resource.h" "$OUT/Resource.h"

echo "== łatki dla clanga (MSVC przepuszcza niekopiowalne obiekty w argumentach ...)"
sed -i 's/self->clientCertificates\[i\]\.Subject,$/self->clientCertificates[i].Subject.get(),/' "$SRC/ClientCertificateSelectionDialog.cpp"
# Przykład zakłada runtime prerelease (interfejsy Experimental); na stabilnym try_query zwraca NULL.
sed -i 's/^    CHECK_FAILURE(exp_env15->add_RestartRequested($/    if (exp_env15) CHECK_FAILURE(exp_env15->add_RestartRequested(/' "$SRC/AppWindow.cpp"

echo "== widl HostObjectSample.idl"
# Biblioteka typów (-t) wywraca widl na tym IDL - pomijamy (dotyczy tylko scenariusza obiektów hosta).
$WIDL -I"$REF/wine/include" -I"$REF/wine-build/include" -m64 \
      -h -H "$OUT/HostObjectSample_h.h" -u -U "$OUT/HostObjectSample_i.c" "$SRC/HostObjectSample.idl"

echo "== wrc (llvm-rc nie parsuje tego .rc)"
$REF/wine-build/tools/wrc/wrc --nostdinc -I"$SRC" -I"$REF/wine/include" -I"$REF/wine-build/include" \
      -o "$OUT/obj/WebView2APISample.res" "$SRC/WebView2APISample.rc"

echo "== clang-cl ($(ls "$SRC"/*.cpp | wc -l) plików)"
compile() {
    local f=$1 o
    o="$OUT/obj/$(basename "${f%.*}").obj"
    [ "$o" -nt "$f" ] && return 0
    # TextInputDialog.h deklaruje `friend DlgProcStatic` -> każdy plik eksportowałby swoją "statyczną"
    # wersję pod tą samą nazwą (MSVC toleruje, lld nie); unikalna nazwa per plik.
    $LLVM/clang-cl /nologo /c /std:c++20 /Zc:char8_t- /MT /EHsc /O1 /Zc:__cplusplus /utf-8 -Wno-everything $DEFS $SYS \
        /D"DlgProcStatic=DlgProcStatic_$(basename "${f%.*}")" \
        /I "$SRC" /I "$OUT" /I "$WV2/include" /I "$WIL" /Fo"$o" "$f" || { echo "BŁĄD: $f"; return 1; }
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
