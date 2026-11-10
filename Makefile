# wine-webview2 — budowanie poza drzewem Wine (jak lsteamclient w Protonie).
#
# Wymaga zbudowanego drzewa Wine (nagłówki, widl, wrc, winegcc, biblioteki importów PE):
#   WINE_SRC   — źródła Wine
#   WINE_BUILD — katalog budowania Wine (configure --enable-archs=x86_64,i386)
#
# make            — DLL x86_64 i i386
# make stubs      — regeneracja szkieletu z IDL (tools/gen-stubs)

WINE_SRC   ?= $(CURDIR)/../wine-webview2-ref/wine
WINE_BUILD ?= $(CURDIR)/../wine-webview2-ref/wine-build

WIDL    = $(WINE_BUILD)/tools/widl/widl
WRC     = $(WINE_BUILD)/tools/wrc/wrc
WINEGCC = $(WINE_BUILD)/tools/winegcc/winegcc

B   = build
DLL = dlls/embeddedbrowserwebview
SRCS = main.c stubs.c

INCLUDES = -I$(B) -I$(DLL) -I$(WINE_BUILD)/include -I$(WINE_SRC)/include -I$(WINE_SRC)/include/msvcrt
CFLAGS   = -O2 -g -Wall -fno-strict-aliasing -D_UCRT -D__WINESRC__ -D__WINE_PE_BUILD \
           -Wno-unused-function
LIBS     = -lole32 -luser32 -lkernel32 -luuid -lucrtbase -lntdll

all: $(B)/x86_64/EmbeddedBrowserWebView.dll $(B)/i386/EmbeddedBrowserWebView.dll

$(B)/webview2.h: include/webview2.idl
	@mkdir -p $(B)
	$(WIDL) -I$(WINE_SRC)/include -I$(WINE_BUILD)/include -h -o $@ $<

$(B)/version.res: $(DLL)/version.rc $(DLL)/webview2_private.h
	@mkdir -p $(B)
	$(WRC) --nostdinc -I$(DLL) -I$(WINE_BUILD)/include -I$(WINE_SRC)/include -o $@ $<

define arch_rules
$(B)/$(1)/%.o: $(DLL)/%.c $(B)/webview2.h $(DLL)/stubs.h $(DLL)/webview2_private.h
	@mkdir -p $(B)/$(1)
	$(2)-gcc -c -o $$@ $$< $(INCLUDES) $(CFLAGS)

$(B)/$(1)/EmbeddedBrowserWebView.dll: $(addprefix $(B)/$(1)/,$(SRCS:.c=.o)) $(B)/version.res $(DLL)/embeddedbrowserwebview.spec
	$(WINEGCC) -o $$@ --wine-objdir $(WINE_BUILD) --cc-cmd="$(2)-gcc" -b $(2) -shared \
	  $(DLL)/embeddedbrowserwebview.spec $(addprefix $(B)/$(1)/,$(SRCS:.c=.o)) $(B)/version.res $(LIBS) \
	  -L$(WINE_BUILD)/libs/winecrt0/$(1)-windows -lwinecrt0
endef

$(eval $(call arch_rules,x86_64,x86_64-w64-mingw32))
$(eval $(call arch_rules,i386,i686-w64-mingw32))

stubs: $(B)/webview2.h
	cd tools/gen-stubs && cargo build --release -q
	tools/gen-stubs/target/release/gen-stubs $(B)/webview2.h include/webview2.idl $(DLL)

clean:
	rm -rf $(B)

.PHONY: all stubs clean
