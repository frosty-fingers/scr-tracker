 =====================================================================
# GBC Release Template — Makefile
# Toolchain: GBDK-2020 (https://github.com/gbdk-2020/gbdk-2020)
#
# Set GBDK_HOME to your install path, e.g.:
#   export GBDK_HOME=/opt/gbdk/
# (must include the trailing slash, or adjust below)
# =====================================================================

GBDK_HOME  ?= /opt/gbdk/
LCC        := $(GBDK_HOME)bin/lcc
PNG2ASSET  := $(GBDK_HOME)bin/png2asset

PROJECT    := sorcerylife
VERSION    := 0.1.0

# Set by CI to the GitHub Actions run number (auto-increments every
# build - see .github/workflows/build-rom.yml) so the ROM filename and
# the version shown on the title screen always match. Local builds
# that don't set this fall back to "local".
BUILD_NUMBER ?= local

SRCDIR     := src
RESDIR     := res
INCDIR     := include
BUILDDIR   := build
THIRDPARTY := third_party
RELEASEDIR := release

# --- Cartridge configuration -----------------------------------------
# This game has no save data, so it deliberately deviates from the
# template default (0x1B = MBC5+RAM+BATTERY) down to ROM ONLY. Flagging
# per CLAUDE.md's rule on changing cart/MBC config: 0x00 = ROM ONLY, no
# RAM, no battery. src/save.c is still in the repo (unused) in case a
# future version of this game wants persistence.
CART_TYPE  := 0x00

# -Wm-yc  = CGB-compatible (runs on GBC, degrades gracefully on DMG)
# -Wm-yC  = CGB-exclusive (GBC only, refuses to boot on original DMG)
COLOR_MODE := -Wm-yc

LCCFLAGS := -Wa-l -Wl-m -Wl-j \
            -Wl-yt$(CART_TYPE) \
            $(COLOR_MODE) \
            -Wm-yn"$(PROJECT)" \
            -DBUILD_NUMBER=$(BUILD_NUMBER) \
            -I$(INCDIR)

SRC  := $(wildcard $(SRCDIR)/*.c) $(wildcard $(THIRDPARTY)/*/*.c)
OBJ  := $(patsubst %.c,$(BUILDDIR)/%.o,$(notdir $(SRC)))
vpath %.c $(SRCDIR) $(THIRDPARTY)/hUGEDriver

.PHONY: all clean release run assets

all: $(BUILDDIR)/$(PROJECT).gbc

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	$(LCC) $(LCCFLAGS) -c -o $@ $<

$(BUILDDIR)/$(PROJECT).gbc: $(OBJ)
	$(LCC) $(LCCFLAGS) -o $@ $(OBJ)

# Convert a PNG in res/gfx to a .c/.h tile source pair.
# Usage: make assets
# No art assets yet - this game's element icons are hand-authored tile
# data directly in src/elements.h. Uncomment/adapt if real art is added.
assets:
	@echo "No PNG assets configured yet - see res/gfx/README.md"
	# $(PNG2ASSET) $(RESDIR)/gfx/player.png -o $(SRCDIR)/player_sprite.c -sw 16 -sh 16

clean:
	rm -rf $(BUILDDIR) $(RELEASEDIR)

# Launch in an emulator for quick testing (expects `bgb` or similar on PATH).
run: all
	bgb $(BUILDDIR)/$(PROJECT).gbc

# Build + package a versioned, checksummed release artifact.
release: clean all
	VERSION=$(VERSION) PROJECT=$(PROJECT) BUILD_NUMBER=$(BUILD_NUMBER) BUILDDIR=$(BUILDDIR) RELEASEDIR=$(RELEASEDIR) \
		bash tools/release.sh