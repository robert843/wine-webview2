#!/bin/bash
# Buduje testową aplikację Tauri 2 pod Windows (x86_64-pc-windows-msvc) na Linuksie: clang-cl + lld-link + xwin.
#   tests/apps/tauri/build.sh   -> build/tauri/x86_64-pc-windows-msvc/release/tauri-hello.exe
# Zmienne: REF (katalog z xwin), LLVM (katalog bin LLVM >= 18)
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../../.." && pwd)
REF=${REF:-$ROOT/../wine-webview2-ref}
LLVM=${LLVM:-/usr/lib/llvm-20/bin}
X=$REF/xwin
T=x86_64_pc_windows_msvc

export PATH="$LLVM:$PATH"
SYS="-imsvc $X/crt/include -imsvc $X/sdk/include/ucrt -imsvc $X/sdk/include/um -imsvc $X/sdk/include/shared"
export CC_$T=clang-cl CXX_$T=clang-cl AR_$T=llvm-lib RC_$T=llvm-rc
export CFLAGS_$T="$SYS" CXXFLAGS_$T="$SYS"
export CARGO_TARGET_X86_64_PC_WINDOWS_MSVC_LINKER=lld-link
export CARGO_TARGET_X86_64_PC_WINDOWS_MSVC_RUSTFLAGS="-Lnative=$X/crt/lib/x86_64 -Lnative=$X/sdk/lib/um/x86_64 -Lnative=$X/sdk/lib/ucrt/x86_64"
# llvm-rc (zasoby Windows: ikona, VERSIONINFO) potrzebuje nagłówków SDK
export INCLUDE="$X/crt/include;$X/sdk/include/ucrt;$X/sdk/include/um;$X/sdk/include/shared"
export CARGO_TARGET_DIR=${CARGO_TARGET_DIR:-$ROOT/build/tauri}

cd "$HERE/src-tauri"
cargo build --release --target x86_64-pc-windows-msvc "$@"
ls -la "$CARGO_TARGET_DIR/x86_64-pc-windows-msvc/release/tauri-hello.exe"
