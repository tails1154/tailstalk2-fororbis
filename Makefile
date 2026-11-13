# TailsTalk 2 for PS4 (OpenOrbis), very early alpha.
#
# Requires the OpenOrbis PS4 toolchain (v0.5.4 release or newer) and clang + lld on the build machine.
#   export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis/PS4Toolchain
#   make

TITLE       := TailsTalk 2
VERSION     := 0.0.1
TITLE_ID    := TTLK00001
CONTENT_ID  := IV0000-TTLK00001_00-TAILSTALK2000000

# Libraries linked into the ELF
LIBS        := -lc -lkernel -lc++ -lSceUserService -lSceVideoOut -lSceAudioOut -lScePad -lSceSysmodule \
               -lSceFreeType -lSDL2 -lSceNet -lSceHttp -lSceSsl -lSceImeDialog

TOOLCHAIN   ?= $(OO_PS4_TOOLCHAIN)
ifeq ($(TOOLCHAIN),)
$(error Set OO_PS4_TOOLCHAIN to the OpenOrbis PS4Toolchain directory)
endif

SRCDIR      := src
BUILDDIR    := build
SOURCES     := $(wildcard $(SRCDIR)/*.cpp)
OBJS        := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SOURCES))
ASSETS      := $(wildcard assets/fonts/*.ttf)
LIBMODULES  := $(wildcard sce_module/*)

CXX         := clang++
LD          := ld.lld
CDIR        := linux
TOOLS       := $(TOOLCHAIN)/bin/$(CDIR)

# PkgTool is a .NET app and aborts without ICU; invariant mode works on any system
export DOTNET_SYSTEM_GLOBALIZATION_INVARIANT := 1

CXXFLAGS    := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c -isysroot $(TOOLCHAIN) \
               -isystem $(TOOLCHAIN)/include -isystem $(TOOLCHAIN)/include/c++/v1
LDFLAGS     := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

all: $(CONTENT_ID).pkg

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILDDIR)/eboot.elf: $(OBJS)
	$(LD) $(OBJS) -o $@ $(LDFLAGS)

eboot.bin: $(BUILDDIR)/eboot.elf
	$(TOOLS)/create-fself -in=$(BUILDDIR)/eboot.elf -out=$(BUILDDIR)/eboot.oelf --eboot "eboot.bin" --paid 0x3800000000000011

sce_sys/param.sfo: Makefile
	$(TOOLS)/PkgTool.Core sfo_new $@
	$(TOOLS)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(TOOLS)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(TOOLS)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(TOOLS)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(TOOLS)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(TOOLS)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(TOOLS)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(TOOLS)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(TOOLS)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(TOOLS)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

pkg.gp4: eboot.bin sce_sys/param.sfo sce_sys/icon0.png $(LIBMODULES) $(ASSETS)
	$(TOOLS)/create-gp4 -out $@ --content-id=$(CONTENT_ID) --files "$^"

$(CONTENT_ID).pkg: pkg.gp4
	$(TOOLS)/PkgTool.Core pkg_build $< .

clean:
	rm -rf $(BUILDDIR) eboot.bin pkg.gp4 $(CONTENT_ID).pkg sce_sys/param.sfo

.PHONY: all clean
