# wine-webview2 — budowanie poza drzewem Wine (jak lsteamclient w Protonie).
#
# Wymaga zbudowanego drzewa Wine (nagłówki, widl, wrc, winegcc, biblioteki importów PE):
#   WINE_SRC   — źródła Wine
#   WINE_BUILD — katalog budowania Wine (configure --enable-archs=x86_64,i386)
#
# make            — DLL x86_64 i i386 + biblioteka uniksowa x86_64 + pomocnik (build/runtime/)
# make stubs      — regeneracja szkieletu z IDL (tools/gen-stubs)
#
# build/runtime/ ma układ docelowej paczki /usr/lib/wine-webview2/: pomocnik + pliki CEF.

WINE_SRC   ?= $(CURDIR)/../wine-webview2-ref/wine
WINE_BUILD ?= $(CURDIR)/../wine-webview2-ref/wine-build

WIDL    = $(WINE_BUILD)/tools/widl/widl
WRC     = $(WINE_BUILD)/tools/wrc/wrc
WINEGCC = $(WINE_BUILD)/tools/winegcc/winegcc

# CEF pobrany przez build.rs crate'a cef (wersja wynika z helper/Cargo.toml)
CEF_PATH    ?= $(CURDIR)/../wine-webview2-ref/cef-rs
CEF_VERSION  = 156.0.26
CEF_DIR      = $(CEF_PATH)/$(CEF_VERSION)/cef_linux_x86_64

B   = build
DLL = dlls/embeddedbrowserwebview
SRCS = main.c controller.c proxy.c proxy_gen.c

INCLUDES = -I$(B) -I$(DLL) -I$(WINE_BUILD)/include -I$(WINE_SRC)/include -I$(WINE_SRC)/include/msvcrt
CFLAGS   = -O2 -g -Wall -fno-strict-aliasing -D_UCRT -D__WINESRC__ -D__WINE_PE_BUILD \
           -Wno-unused-function
LIBS     = -lole32 -luser32 -lgdi32 -lshlwapi -luxtheme -lkernel32 -luuid -lucrtbase -lntdll

UNIX_CFLAGS = -O2 -g -Wall -fPIC -fno-strict-aliasing -fvisibility=hidden -DWINE_UNIX_LIB -D__WINESRC__ \
              -I$(DLL) -I$(WINE_BUILD)/include -I$(WINE_SRC)/include

all: $(B)/x86_64/EmbeddedBrowserWebView.dll $(B)/i386/EmbeddedBrowserWebView.dll \
     $(B)/x86_64/embeddedbrowserwebview.so $(B)/x86_64/msedgewebview2.exe runtime

# Atrapa procesu przeglądarki (aplikacje sprawdzają, czy plik istnieje w katalogu runtime'u).
$(B)/x86_64/msedgewebview2.exe: programs/msedgewebview2/main.c $(B)/version.res
	@mkdir -p $(B)/x86_64
	x86_64-w64-mingw32-gcc -c -o $(B)/x86_64/msedgewebview2.o $< $(INCLUDES) $(CFLAGS)
	$(WINEGCC) -o $@ --wine-objdir $(WINE_BUILD) --cc-cmd="x86_64-w64-mingw32-gcc" -b x86_64-w64-mingw32 \
	  -mwindows -municode $(B)/x86_64/msedgewebview2.o $(B)/version.res -lkernel32 -lucrtbase -lntdll \
	  -L$(WINE_BUILD)/libs/winecrt0/x86_64-windows -lwinecrt0 -lkernel32 -lntdll

# Część uniksowa: ładowana przez __wine_load_unix_lib (Wine >= 11.2) z katalogu DLL-ki.
$(B)/x86_64/embeddedbrowserwebview.so: $(DLL)/unixlib.c $(DLL)/unixlib.h $(DLL)/protocol.h
	@mkdir -p $(B)/x86_64
	gcc -shared -o $@ $< $(UNIX_CFLAGS) -L$(WINE_BUILD)/dlls/ntdll -l:ntdll.so -Wl,-z,defs -lpthread

# Pomocnik (Rust + CEF) i katalog uruchomieniowy z plikami CEF.
runtime:
	cd helper && CEF_PATH=$(CEF_PATH) cargo build --release
	@mkdir -p $(B)/runtime
	for f in $(CEF_DIR)/*.so $(CEF_DIR)/*.so.1 $(CEF_DIR)/*.pak $(CEF_DIR)/*.dat $(CEF_DIR)/*.bin \
	         $(CEF_DIR)/*.json $(CEF_DIR)/locales; do ln -sfn $$f $(B)/runtime/; done
	cp helper/target/release/wine-webview2-helper $(B)/runtime/

$(B)/webview2.h: include/webview2.idl
	@mkdir -p $(B)
	$(WIDL) -I$(WINE_SRC)/include -I$(WINE_BUILD)/include -h -o $@ $<

$(B)/version.res: $(DLL)/version.rc $(DLL)/webview2_private.h
	@mkdir -p $(B)
	$(WRC) --nostdinc -I$(DLL) -I$(WINE_BUILD)/include -I$(WINE_SRC)/include -o $@ $<

define arch_rules
$(B)/$(1)/%.o: $(DLL)/%.c $(B)/webview2.h $(DLL)/webview2_private.h $(DLL)/unixlib.h $(DLL)/protocol.h $(DLL)/proxy.h $(DLL)/proxy_gen.h
	@mkdir -p $(B)/$(1)
	$(2)-gcc -c -o $$@ $$< $(INCLUDES) $(CFLAGS)

$(B)/$(1)/EmbeddedBrowserWebView.dll: $(addprefix $(B)/$(1)/,$(SRCS:.c=.o)) $(B)/version.res $(DLL)/embeddedbrowserwebview.spec
	$(WINEGCC) -o $$@ --wine-objdir $(WINE_BUILD) --cc-cmd="$(2)-gcc" -b $(2) -shared \
	  $(DLL)/embeddedbrowserwebview.spec $(addprefix $(B)/$(1)/,$(SRCS:.c=.o)) $(B)/version.res $(LIBS) \
	  -L$(WINE_BUILD)/libs/winecrt0/$(1)-windows -lwinecrt0 -lntdll
endef

$(eval $(call arch_rules,x86_64,x86_64-w64-mingw32))
$(eval $(call arch_rules,i386,i686-w64-mingw32))

stubs: $(B)/webview2.h
	cd tools/gen-stubs && cargo build --release -q
	tools/gen-stubs/target/release/gen-stubs $(B)/webview2.h include/webview2.idl $(DLL) helper/src/api_gen.rs

clean:
	rm -rf $(B)

.PHONY: all stubs clean runtime
