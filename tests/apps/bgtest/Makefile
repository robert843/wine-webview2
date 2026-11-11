WV2_SDK ?= $(error set WV2_SDK to the unpacked Microsoft.Web.WebView2 NuGet package)
CXX = x86_64-w64-mingw32-g++
all: bgtest.exe WebView2Loader.dll
bgtest.exe: bgtest.cc
	$(CXX) -std=c++17 -O1 -g -I../hello/compat -I$(WV2_SDK)/build/native/include -static -o $@ $< -lole32 -luser32 -lgdi32
WebView2Loader.dll: $(WV2_SDK)/build/native/x64/WebView2Loader.dll
	cp $< $@
