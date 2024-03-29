#!/usr/bin/make -f
OPTIMIZATIONS ?= -msse -msse2 -mfpmath=sse -ffast-math -fomit-frame-pointer -O3 -fno-finite-math-only -DNDEBUG
#OPTIMIZATIONS ?= -mtune=cortex-a53 -funsafe-math-optimizations -ffast-math -fomit-frame-pointer -O3 -fno-finite-math-only -fvisibility=hidden -fdata-sections -ffunction-sections  -DNDEBUG # -fopt-info-vec-optimize
PREFIX ?= /usr
CFLAGS ?= $(OPTIMIZATIONS) -Wall
URI_PREFIX='"http://polyeffects.com/lv2/basic_modular"'

PKG_CONFIG?=pkg-config
STRIP?=strip
STRIPFLAGS?=-s

poly_filter_VERSION?=$(shell git describe --tags HEAD 2>/dev/null | sed 's/-g.*$$//;s/^v//' || echo "LV2")
###############################################################################

LV2DIR ?= $(PREFIX)/lib/lv2
LOADLIBES=-lm
LV2NAME=basic_modular
BUNDLE=basic_modular.lv2
BUILDDIR=build/
targets=

UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
  LV2LDFLAGS=-dynamiclib
  LIB_EXT=.dylib
  EXTENDED_RE=-E
  STRIPFLAGS=-u -r -arch all -s lv2syms
  targets+=lv2syms
else
  LV2LDFLAGS=-Wl,-Bstatic -Wl,-Bdynamic
  LIB_EXT=.so
  EXTENDED_RE=-r
endif

ifneq ($(XWIN),)
  CXX=$(XWIN)-gcc
  STRIP=$(XWIN)-strip
  LV2LDFLAGS=-Wl,-Bstatic -Wl,-Bdynamic -Wl,--as-needed
  LIB_EXT=.dll
  override LDFLAGS += -static-libgcc -static-libstdc++
else
  override CFLAGS += -fPIC -fvisibility=hidden
endif

targets+=$(BUILDDIR)$(LV2NAME)$(LIB_EXT)
targets+=$(BUILDDIR)maths$(LIB_EXT)
targets+=$(BUILDDIR)clock_divider$(LIB_EXT)

###############################################################################
# extract versions
LV2VERSION=1

# check for build-dependencies
ifeq ($(shell $(PKG_CONFIG) --exists lv2 || echo no), no)
  $(error "LV2 SDK was not found")
endif

override CFLAGS += -lstdc++ `$(PKG_CONFIG) --cflags lv2`

# build target definitions
default: all

all: $(BUILDDIR)manifest.ttl $(BUILDDIR)difference.ttl $(BUILDDIR)amp.ttl $(targets)

lv2syms:
	echo "_lv2_descriptor" > lv2syms

$(BUILDDIR)manifest.ttl: manifest.ttl
	@mkdir -p $(BUILDDIR)
	cat manifest.ttl > $(BUILDDIR)manifest.ttl

$(BUILDDIR)amp.ttl: amp.ttl 
	@mkdir -p $(BUILDDIR)
	cat amp.ttl > $(BUILDDIR)amp.ttl

$(BUILDDIR)difference.ttl: difference.ttl 
	@mkdir -p $(BUILDDIR)
	cat difference.ttl > $(BUILDDIR)difference.ttl

$(BUILDDIR)$(LV2NAME)$(LIB_EXT): amp.c 
	@mkdir -p $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) \
	  -o $(BUILDDIR)$(LV2NAME)$(LIB_EXT) amp.c \
		-shared -DURI_PREFIX=$(URI_PREFIX) $(LV2LDFLAGS) $(LDFLAGS) $(LOADLIBES)
	$(STRIP) $(STRIPFLAGS) $(BUILDDIR)$(LV2NAME)$(LIB_EXT)


$(BUILDDIR)maths$(LIB_EXT): difference.c 
	@mkdir -p $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) \
	  -o $(BUILDDIR)maths$(LIB_EXT) difference.c \
		-shared -DURI_PREFIX=$(URI_PREFIX) $(LV2LDFLAGS) $(LDFLAGS) $(LOADLIBES)
	$(STRIP) $(STRIPFLAGS) $(BUILDDIR)$(LV2NAME)$(LIB_EXT)

$(BUILDDIR)clock_divider$(LIB_EXT): clock_divider.c 
	@mkdir -p $(BUILDDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) \
	  -o $(BUILDDIR)clock_divider$(LIB_EXT) clock_divider.c \
		-shared -DURI_PREFIX=$(URI_PREFIX) $(LV2LDFLAGS) $(LDFLAGS) $(LOADLIBES)
	$(STRIP) $(STRIPFLAGS) $(BUILDDIR)$(LV2NAME)$(LIB_EXT)
# install/uninstall/clean target definitions

install: all
	install -d $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m755 $(BUILDDIR)$(LV2NAME)$(LIB_EXT) $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m755 $(BUILDDIR)maths$(LIB_EXT) $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m755 $(BUILDDIR)clock_divider$(LIB_EXT) $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m644 $(BUILDDIR)manifest.ttl $(BUILDDIR)difference.ttl $(BUILDDIR)amp.ttl $(DESTDIR)$(LV2DIR)/$(BUNDLE)

uninstall:
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/manifest.ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME).ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME)$(LIB_EXT)
	-rmdir $(DESTDIR)$(LV2DIR)/$(BUNDLE)

clean:
	rm -f $(BUILDDIR)manifest.ttl $(BUILDDIR)$(LV2NAME).ttl $(BUILDDIR)$(LV2NAME)$(LIB_EXT) lv2syms
	-test -d $(BUILDDIR) && rmdir $(BUILDDIR) || true

.PHONY: clean all install uninstall
