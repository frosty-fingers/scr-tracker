# Setup

## 1. Install GBDK-2020

Download the latest release for your OS from:
https://github.com/gbdk-2020/gbdk-2020/releases

Unzip it somewhere stable, then point `GBDK_HOME` at it (note the trailing slash):

```bash
export GBDK_HOME=/opt/gbdk/
```

Add that line to your shell profile so it persists across sessions. On Windows, set it as
a system/user environment variable instead.

Verify it's on your PATH-adjacent setup by running:

```bash
$GBDK_HOME/bin/lcc -v
```

## 2. Build the template

```bash
make          # builds build/mygame.gbc
make run      # builds + launches in an emulator (expects `bgb` or similar on PATH)
```

## 3. An emulator for testing

[BGB](https://bgb.bircd.org/) (Windows, runs fine under Wine) and
[Emulicious](https://emulicious.net/) (cross-platform, has a debugger with
VRAM/OAM viewers) are the two most commonly used for GBC development —
both show hardware quirks real cartridges hit that more "accurate for
players" emulators smooth over.

## 4. Graphics pipeline

Art goes in `res/gfx/` as PNGs. `png2asset` (bundled with GBDK-2020) converts
them into `.c`/`.h` tile data. The Makefile's `assets` target has one example
rule — add a line per sprite/background sheet as you add art:

```bash
make assets
```

Tile size, palette count, and export mode all depend on whether it's a
background tile sheet or a sprite metasprite — see the GBDK docs on
`png2asset` for the flags: https://gbdk-2020.github.io/gbdk-2020/docs/

## 5. Music: hUGEDriver

This template's `src/music.c` is a thin wrapper around
[hUGEDriver](https://github.com/SuperDisk/hUGEDriver), a widely-used
GB/GBC sound driver.

1. Clone hUGEDriver and copy its `src/` contents into `third_party/hUGEDriver/`.
2. Compose in [hUGETracker](https://github.com/SuperDisk/hUGETracker) and
   export your song as a `.c`/`.h` pair into `res/music/`.
3. In `src/music.c`, uncomment the `#include`s and the `hUGE_init()` /
   `hUGE_dosound()` calls, pointing at your exported song.
4. Check hUGEDriver's own Makefile/README for whether it's built in
   interrupt-driven mode (no polling needed) or polled mode (needs
   `music_update()` called every frame, which `main.c` already does).

## 6. Battery saves

Already wired up in `src/save.c`. The cartridge type in the Makefile
(`CART_TYPE := 0x1B`) is MBC5+RAM+BATTERY, which is what actually makes
saves survive a power-off — MBC5+RAM alone does not. If you emulate before
flashing real hardware, confirm your emulator/flash cart correctly reports
this cart type so it emulates the battery-backed SRAM.

## 7. Release checklist

Before shipping a ROM:

- [ ] Bump `VERSION` in the Makefile
- [ ] Bump `SAVE_VERSION` in `src/save.h` if the save layout changed
  (prevents old saves from being misread as valid)
- [ ] `make clean && make release`
- [ ] Test the packaged ROM in at least one emulator AND, if targeting
  real hardware, on an actual flash cart
- [ ] Keep the `.sym` file from `release/` for future debugging of bug
  reports against that exact release build
