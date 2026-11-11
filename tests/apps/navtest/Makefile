WV2_SDK ?= $(error set WV2_SDK to the unpacked Microsoft.Web.WebView2 NuGet package)
CXX = x86_64-w64-mingw32-g++
all: navtest.exe WebView2Loader.dll x86/navtest.exe x86/WebView2Loader.dll
navtest.exe: navtest.cc
	$(CXX) -std=c++17 -O1 -g -I../hello/compat -I$(WV2_SDK)/build/native/include -static -o $@ $< -lole32 -luser32 -lgdi32
WebView2Loader.dll: $(WV2_SDK)/build/native/x64/WebView2Loader.dll
	cp $< $@

# 32-bit variant (separate directory: the loader DLL has the same name).
x86/navtest.exe: navtest.cc
	@mkdir -p x86
	i686-w64-mingw32-g++ -std=c++17 -O1 -g -I../hello/compat -I$(WV2_SDK)/build/native/include -static -o $@ $< -lole32 -luser32 -lgdi32
x86/WebView2Loader.dll: $(WV2_SDK)/build/native/x86/WebView2Loader.dll
	@mkdir -p x86
	cp $< $@
