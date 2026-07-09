# GBC Release Template

A starting point for Game Boy Color homebrew projects, built on
[GBDK-2020](https://github.com/gbdk-2020/gbdk-2020). Covers the parts that
are easy to get wrong or skip until it's too late: correct cartridge/MBC
configuration, battery-backed saves, a music driver hookup, and a
one-command release packaging step.

See **docs/SETUP.md** for full setup instructions (install the toolchain
first — nothing here builds without it).

## Structure

```
src/            Game source (.c/.h)
res/gfx/        Sprite & background art (PNGs, converted via `make assets`)
res/music/      hUGETracker song exports
res/maps/       Tilemap data
third_party/    Vendored libraries (hUGEDriver goes here)
tools/          Release packaging script
docs/           Setup & workflow docs
build/          Build output (gitignored)
release/        Packaged release artifacts (gitignored)
```

## Quick start

```bash
export GBDK_HOME=/opt/gbdk/     # wherever you installed GBDK-2020
make                            # build/mygame.gbc
make run                        # build + open in an emulator
make release                    # versioned, checksummed .zip in release/
```

## Building without a local compiler (phone-friendly)

`.github/workflows/build-rom.yml` builds the ROM on every push to `main`
(or manually from the Actions tab) and uploads the `.gbc` as a downloadable
artifact. Useful if you're coding from a phone or any machine without
GBDK-2020 installed: push your changes, wait a couple minutes, download
the ROM from the Actions run, and load it into an emulator (e.g. Delta on
iOS) to test.

## CLAUDE.md

Project-specific ground rules for Claude — hardware constraints, the
phone-first no-local-compiler workflow, save versioning, code style. Claude
Code reads this automatically; if you're working in a claude.ai Project,
paste its contents into the Project's custom instructions.

## What's already wired up

- **Cartridge config**: MBC5+RAM+BATTERY (`0x1B`), CGB-compatible mode
  (`-Wm-yc`) so it runs on GBC and degrades sanely on original DMG.
- **Save system** (`src/save.c`): versioned struct + checksum in
  battery-backed SRAM, so a version mismatch or corrupted save is
  rejected instead of loaded as garbage.
- **Music** (`src/music.c`): thin wrapper around hUGEDriver — vendor the
  driver and drop in an exported song, no need to touch `main.c`.
- **Release script** (`tools/release.sh`): stamps a version + date,
  generates a sha256 checksum, zips the ROM with its `.sym` file for
  post-release debugging.

## Using Claude on this project

This template is meant to be a stable base you hand to Claude for
feature work — e.g. "add a title screen that reads the save and shows
a Continue option" or "add a second sprite with collision against the
background tilemap." Keeping the save/music/build plumbing already
correct means Claude's changes stay scoped to gameplay code rather than
re-deriving MBC flags or SRAM timing each time.
