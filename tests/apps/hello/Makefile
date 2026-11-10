# hello-builtin.exe — loader wbudowany w webview/webview (własna reimplementacja)
# hello-loader.exe  — oficjalny WebView2Loader.dll z SDK (ładowany w czasie działania)
WEBVIEW_SRC ?= $(error ustaw WEBVIEW_SRC na klon github.com/webview/webview)
WV2_SDK     ?= $(error ustaw WV2_SDK na rozpakowany NuGet Microsoft.Web.WebView2)
CXX = x86_64-w64-mingw32-g++
CXXFLAGS = -std=c++17 -O1 -g -Icompat -I$(WEBVIEW_SRC)/core/include -I$(WV2_SDK)/build/native/include \
           -mwindows -static
LIBS = -ladvapi32 -lole32 -lshell32 -lshlwapi -luser32 -lversion -ldwmapi

all: hello-builtin.exe hello-loader.exe WebView2Loader.dll

hello-builtin.exe: hello.cc
	$(CXX) $(CXXFLAGS) -DWEBVIEW_MSWEBVIEW2_BUILTIN_IMPL=1 -o $@ $< $(LIBS)

hello-loader.exe: hello.cc
	$(CXX) $(CXXFLAGS) -DWEBVIEW_MSWEBVIEW2_BUILTIN_IMPL=0 -DWEBVIEW_MSWEBVIEW2_EXPLICIT_LINK=1 -o $@ $< $(LIBS)

WebView2Loader.dll: $(WV2_SDK)/build/native/x64/WebView2Loader.dll
	cp $< $@

clean:
	rm -f *.exe WebView2Loader.dll
