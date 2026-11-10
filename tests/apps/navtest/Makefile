WV2_SDK ?= $(error ustaw WV2_SDK na rozpakowany NuGet Microsoft.Web.WebView2)
CXX = x86_64-w64-mingw32-g++
all: navtest.exe WebView2Loader.dll
navtest.exe: navtest.cc
	$(CXX) -std=c++17 -O1 -g -I../hello/compat -I$(WV2_SDK)/build/native/include -static -o $@ $< -lole32 -luser32
WebView2Loader.dll: $(WV2_SDK)/build/native/x64/WebView2Loader.dll
	cp $< $@
