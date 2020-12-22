INCLUDES=
CPPFLAGS=-s -O3 -ffast-math -fPIC
#-g for debug
#-s -O3 -ffast-math for release
LIBS=
LV2_OBJS = lpc.o lpc_plugin.o

BUILD_UI?=yes

RW ?= robtk/
RT=$(RW)rtk/
PUGL_SRC=$(RW)pugl/pugl_x11.c
GLUILIBS = -lX11 $(shell pkg-config --libs cairo pango pangocairo glu gl)
GLUICFLAGS = -DUSE_GUI_THREAD $(shell pkg-config --cflags cairo pango)
LV2LDFLAGS=-Wl,-Bstatic -Wl,-Bdynamic -Wl,--as-needed -pthread
ROBGL= $(RW)robtk.mk $(UITOOLKIT) $(RW)ui_gl.c $(PUGL_SRC) \
  $(RW)gl/common_cgl.h $(RW)gl/layout.h $(RW)gl/robwidget_gl.h $(RW)robtk.h \
	$(RT)common.h $(RT)style.h \
  $(RW)gl/xternalui.c $(RW)gl/xternalui.h

LV2_BIN=lpc_plugin
LV2_BUNDLE=lpc.lv2/
UI_BIN=lpcUI_gl
LIB_EXT=.so
INSTALLDIR=~/.lv2/
LV2_URI=http://example.com/plugins/lpc_plugin
UI_TYPE=ui:X11UI
UI_SUFFIX=\#ui_gl
UI_LINK=
TTL_SUB="s%@LIB_EXT@%$(LIB_EXT)%;s%@LV2_BIN@%$(LV2_BIN)%;s%@LV2_URI@%$(LV2_URI)%;s%@UI_BIN@%$(UI_BIN)%;s%@UI_TYPE@%$(UI_TYPE)%;s%@UI_SUFFIX@%$(UI_SUFFIX)%;s%@UI_LINK@%$(UI_LINK)%"
CPPFLAGS+=-DLPC_URI="\"$(LV2_URI)\""

MANIFEST_TTLS=manifest.ttl.in
PLUGIN_TTLS=$(LV2_BIN).ttl.in

ALL=$(LV2_BUNDLE)$(LV2_BIN)$(LIB_EXT) $(LV2_BUNDLE)manifest.ttl $(LV2_BUNDLE)$(LV2_BIN).ttl

ifeq ($(BUILD_UI), yes)
ALL+=$(LV2_BUNDLE)$(UI_BIN)$(LIB_EXT)
MANIFEST_TTLS+=manifest_ui.ttl.in
PLUGIN_TTLS+=$(LV2_BIN)_ui.ttl.in
UI_LINK=ui:ui <$(LV2_URI)$(UI_SUFFIX)> ;
endif

all: $(ALL)

$(LV2_BUNDLE):
	mkdir $(LV2_BUNDLE)

$(LV2_BUNDLE)$(LV2_BIN)$(LIB_EXT): $(LV2_OBJS) $(LV2_BUNDLE)
	$(CXX) $(CPPFLAGS) -shared -o $@ $(LV2_OBJS) $(LIBS)


$(LV2_BUNDLE)$(UI_BIN)$(LIB_EXT): $(ROBGL) $(LV2_BUNDLE)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(GLUICFLAGS) $(PTHREADCFLAGS) \
	  -DUINQHACK="$(shell date +%s$$$$)" \
	  -DPLUGIN_SOURCE="\"../lpc_ui.cpp\"" \
	  -o $@ $(RW)ui_gl.c \
	  $(PUGL_SRC) \
	  $(value $(*F)_UISRC) \
	  -shared $(LV2LDFLAGS) $(LDFLAGS) $(GLUILIBS)

$(LV2_BUNDLE)manifest.ttl: $(LV2_BUNDLE) $(MANIFEST_TTLS)
	sed $(TTL_SUB) $(MANIFEST_TTLS) > $@
	

$(LV2_BUNDLE)$(LV2_BIN).ttl: $(LV2_BUNDLE) $(PLUGIN_TTLS)
	sed $(TTL_SUB) $(PLUGIN_TTLS) > $@

clean: 
	rm -r $(LV2_BUNDLE)
	rm *.o

install: all
	mkdir -p $(INSTALLDIR)
	cp -r $(LV2_BUNDLE) $(INSTALLDIR)

uninstall:
	rm $(INSTALLDIR)$(LV2_BUNDLE)*.so
	rm $(INSTALLDIR)$(LV2_BUNDLE)*.ttl
	rmdir $(INSTALLDIR)$(LV2_BUNDLE)

validate: $(LV2_BUNDLE)manifest.ttl $(LV2_BUNDLE)$(LV2_BIN).ttl
	sord_validate -l $(shell find -L /usr/include/lv2 /usr/lib/lv2/schemas.lv2 -type f -name '*.ttl') $(LV2_BUNDLE)manifest.ttl $(LV2_BUNDLE)$(LV2_BIN).ttl
	
.PHONY: all clean install uninstall validate

