# wine-webview2 - out-of-tree build against Wine (like lsteamclient in Proton).
#
# Either an installed Wine with development files (e.g. WineHQ's wine-devel + wine-devel-dev packages):
#   WINE_INSTALL - installation prefix, e.g. /opt/wine-devel
# or a built Wine tree:
#   WINE_SRC     - Wine sources
#   WINE_BUILD   - Wine build directory (configure --enable-archs=x86_64,i386)
#
# make        - x86_64 and i386 DLLs + x86_64 unix library + helper (build/runtime/)
# make stubs  - regenerate the generated code from the IDL (tools/gen-stubs)
# make dist   - release tarball (VERSION=x.y.z overrides the version)
# make check  - run the test programs in a test prefix (needs WV2_SDK, see tests/check.sh)
#
# build/runtime/ has the layout of the installed helper directory: helper + CEF files.

REF        ?= $(CURDIR)/../wine-webview2-ref

ifdef WINE_INSTALL
WIDL        = $(WINE_INSTALL)/bin/widl
WRC         = $(WINE_INSTALL)/bin/wrc
WINEGCC     = $(WINE_INSTALL)/bin/winegcc
WINE_INCS   = -I$(WINE_INSTALL)/include/wine/windows -I$(WINE_INSTALL)/include -I$(CURDIR)/third_party/wine/include
WINE_MSVCRT = -I$(WINE_INSTALL)/include/wine/msvcrt
OBJDIR_FLAG =
crt0_dir    = $(WINE_INSTALL)/lib/wine/$(1)-windows
NTDLL_UNIX  = $(WINE_INSTALL)/lib/wine/x86_64-unix
# Old (non-WoW64) 32-bit support: a 32-bit unix library when the installation has a 32-bit unix side.
NTDLL_UNIX32 = $(wildcard $(WINE_INSTALL)/lib/wine/i386-unix/ntdll.so)
else
WINE_SRC   ?= $(REF)/wine
WINE_BUILD ?= $(REF)/wine-build
WIDL        = $(WINE_BUILD)/tools/widl/widl
WRC         = $(WINE_BUILD)/tools/wrc/wrc
WINEGCC     = $(WINE_BUILD)/tools/winegcc/winegcc
WINE_INCS   = -I$(WINE_BUILD)/include -I$(WINE_SRC)/include
WINE_MSVCRT = -I$(WINE_SRC)/include/msvcrt
OBJDIR_FLAG = --wine-objdir $(WINE_BUILD)
crt0_dir    = $(WINE_BUILD)/libs/winecrt0/$(1)-windows
NTDLL_UNIX  = $(WINE_BUILD)/dlls/ntdll
endif

# CEF downloaded by the cef crate's build.rs (version follows helper/Cargo.toml)
CEF_PATH    ?= $(REF)/cef-rs
CEF_VERSION  = 156.0.26
CEF_DIR      = $(CEF_PATH)/$(CEF_VERSION)/cef_linux_x86_64

B   = build
DLL = dlls/embeddedbrowserwebview
SRCS = main.c controller.c proxy.c proxy_gen.c

INCLUDES = -I$(B) -I$(DLL) $(WINE_INCS) $(WINE_MSVCRT)
CFLAGS   = -O2 -g -Wall -fno-strict-aliasing -D_UCRT -D__WINESRC__ -D__WINE_PE_BUILD \
           -Wno-unused-function
LIBS     = -lole32 -lshell32 -lcomdlg32 -luser32 -lgdi32 -lshlwapi -luxtheme -lkernel32 -luuid -lucrtbase -lntdll

UNIX_CFLAGS = -O2 -g -Wall -fPIC -fno-strict-aliasing -fvisibility=hidden -DWINE_UNIX_LIB -D__WINESRC__ \
              -I$(DLL) $(WINE_INCS)

UNIX32 = $(if $(NTDLL_UNIX32),$(B)/i386/embeddedbrowserwebview.so)

all: $(B)/x86_64/EmbeddedBrowserWebView.dll $(B)/i386/EmbeddedBrowserWebView.dll \
     $(B)/x86_64/embeddedbrowserwebview.so $(UNIX32) $(B)/x86_64/msedgewebview2.exe runtime

# Dummy browser process: apps check that the file exists in the runtime directory.
$(B)/x86_64/msedgewebview2.exe: programs/msedgewebview2/main.c $(B)/version.res
	@mkdir -p $(B)/x86_64
	x86_64-w64-mingw32-gcc -c -o $(B)/x86_64/msedgewebview2.o $< $(INCLUDES) $(CFLAGS)
	$(WINEGCC) -o $@ $(OBJDIR_FLAG) --cc-cmd="x86_64-w64-mingw32-gcc" -b x86_64-w64-mingw32 \
	  -mwindows -municode $(B)/x86_64/msedgewebview2.o $(B)/version.res -lkernel32 -lucrtbase -lntdll \
	  -L$(call crt0_dir,x86_64) -lwinecrt0 -lkernel32 -lntdll

# Unix side, loaded by explicit path from the DLL's directory (Wine >= 11.2).
$(B)/x86_64/embeddedbrowserwebview.so: $(DLL)/unixlib.c $(DLL)/unixlib.h $(DLL)/protocol.h
	@mkdir -p $(B)/x86_64
	gcc -shared -o $@ $< $(UNIX_CFLAGS) -L$(NTDLL_UNIX) -l:ntdll.so -Wl,-z,defs -lpthread

# 32-bit unix side for the old WoW64-less mode (in the new mode the x86 DLL uses the 64-bit library).
$(B)/i386/embeddedbrowserwebview.so: $(DLL)/unixlib.c $(DLL)/unixlib.h $(DLL)/protocol.h
	@mkdir -p $(B)/i386
	gcc -m32 -shared -o $@ $< $(UNIX_CFLAGS) -L$(dir $(NTDLL_UNIX32)) -l:ntdll.so -Wl,-z,defs -lpthread

# Helper (Rust + CEF) and the runtime directory with the CEF files.
runtime:
	cd helper && CEF_PATH=$(CEF_PATH) cargo build --release
	@mkdir -p $(B)/runtime
	for f in $(CEF_DIR)/*.so $(CEF_DIR)/*.so.1 $(CEF_DIR)/*.pak $(CEF_DIR)/*.dat $(CEF_DIR)/*.bin \
	         $(CEF_DIR)/*.json $(CEF_DIR)/locales; do ln -sfn $$f $(B)/runtime/; done
	cp helper/target/release/wine-webview2-helper $(B)/runtime/.wine-webview2-helper.new && mv -f $(B)/runtime/.wine-webview2-helper.new $(B)/runtime/wine-webview2-helper
	@# Accessibility libraries CEF needs that the Steam Linux Runtime 4 container (Proton) lacks.
	@for l in libatk-bridge-2.0.so.0 libatspi.so.0; do \
	    src=$$(ldconfig -p | awk -v L=$$l '$$1==L && /x86-64/ {print $$NF; exit}'); \
	    if [ -n "$$src" ]; then cp -L "$$src" $(B)/runtime/$$l; else echo "warning: $$l not found on the system"; fi; done

$(B)/webview2.h: include/webview2.idl
	@mkdir -p $(B)
	$(WIDL) $(WINE_INCS) -h -o $@ $<

$(B)/version.res: $(DLL)/version.rc $(DLL)/webview2_private.h
	@mkdir -p $(B)
	$(WRC) --nostdinc -I$(DLL) $(WINE_INCS) -o $@ $<

define arch_rules
$(B)/$(1)/%.o: $(DLL)/%.c $(B)/webview2.h $(DLL)/webview2_private.h $(DLL)/unixlib.h $(DLL)/protocol.h $(DLL)/proxy.h $(DLL)/proxy_gen.h
	@mkdir -p $(B)/$(1)
	$(2)-gcc -c -o $$@ $$< $(INCLUDES) $(CFLAGS)

$(B)/$(1)/EmbeddedBrowserWebView.dll: $(addprefix $(B)/$(1)/,$(SRCS:.c=.o)) $(B)/version.res $(DLL)/embeddedbrowserwebview.spec
	$(WINEGCC) -o $$@ $(OBJDIR_FLAG) --cc-cmd="$(2)-gcc" -b $(2) -shared \
	  $(DLL)/embeddedbrowserwebview.spec $(addprefix $(B)/$(1)/,$(SRCS:.c=.o)) $(B)/version.res $(LIBS) \
	  -L$(call crt0_dir,$(1)) -lwinecrt0 -lntdll
endef

$(eval $(call arch_rules,x86_64,x86_64-w64-mingw32))
$(eval $(call arch_rules,i386,i686-w64-mingw32))

stubs: $(B)/webview2.h
	cd tools/gen-stubs && cargo build --release -q
	tools/gen-stubs/target/release/gen-stubs $(B)/webview2.h include/webview2.idl $(DLL) helper/src/api_gen.rs

clean:
	rm -rf $(B)

.PHONY: all stubs clean runtime dist check

check: all
	tests/check.sh

# Release tarball wine-webview2-<version>.tar.xz: script + stripped helper + prefix files.
VERSION ?= $(shell sed -n 's/^VERSION=//p' wine-webview2)
DIST    = $(B)/dist/wine-webview2-$(VERSION)
dist: all
	rm -rf $(DIST) && mkdir -p $(DIST)/helper $(DIST)/prefix/x64 $(DIST)/prefix/x86
	cp README.md LICENSE $(DIST)/
	sed 's/^VERSION=.*/VERSION=$(VERSION)/' wine-webview2 > $(DIST)/wine-webview2 && chmod +x $(DIST)/wine-webview2
	cp -rL $(B)/runtime/. $(DIST)/helper/
	rm -rf $(DIST)/helper/extensions $(DIST)/helper/archive.json
	strip --strip-unneeded $(DIST)/helper/libcef.so $(DIST)/helper/wine-webview2-helper
	cp third_party/CEF-LICENSE.txt $(DIST)/helper/
	cp -L $(CEF_DIR)/CREDITS.html $(DIST)/helper/CHROMIUM-CREDITS.html
	cp $(B)/x86_64/EmbeddedBrowserWebView.dll $(B)/x86_64/embeddedbrowserwebview.so $(DIST)/prefix/x64/
	cp $(B)/i386/EmbeddedBrowserWebView.dll $(DIST)/prefix/x86/
	[ ! -f $(B)/i386/embeddedbrowserwebview.so ] || cp $(B)/i386/embeddedbrowserwebview.so $(DIST)/prefix/x86/
	cp $(B)/x86_64/msedgewebview2.exe $(DIST)/prefix/
	tar -C $(B)/dist -cJf $(B)/dist/wine-webview2-$(VERSION).tar.xz wine-webview2-$(VERSION)
	@ls -la $(B)/dist/wine-webview2-$(VERSION).tar.xz
