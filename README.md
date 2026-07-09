# Sorcery: Contested Realm - Life & Threshold Tracker

A Game Boy Color homebrew ROM (built on the GBDK-2020 project template)
for tracking Life (20 -> Death's Door at 0) and the four elemental
thresholds (Fire, Water, Earth, Air) during a game of *Sorcery:
Contested Realm*.

No save data, no music - this is meant to be reset fresh each game.

## Controls

| Button      | Action                                     |
|-------------|---------------------------------------------|
| LEFT/RIGHT  | Move the cursor between LIFE and elements   |
| A           | +1 to the selected counter                  |
| B           | -1 to the selected counter                  |
| START       | Reset (LIFE to 20, elements to 0)           |

## Building

Push to `main` (or open a PR) to trigger `.github/workflows/build-rom.yml`,
which downloads GBDK-2020 4.5.0 and builds the ROM. Grab it from the
Actions tab -> latest run -> Artifacts.

To build locally with GBDK-2020 installed:

```
make GBDK_HOME=/path/to/gbdk/
```

## Notes for this project specifically

- **Cart type is ROM ONLY (0x00)**, not the template's default
  MBC5+RAM+BATTERY (0x1B) - there's nothing to save. See the Makefile.
- **`src/save.c`/`src/save.h` and `src/music.c`/`src/music.h`** are still
  here from the template but unused/uncalled - left in case a future
  version wants persistence or music.
- **Element icons are original pixel art** (flame / droplet / mountain /
  wind-swirl) in `src/elements.h`, not the game's official threshold
  symbols - those are Sorcery TCG's copyrighted artwork.
- See `docs/GOTCHAS.md` for hardware/toolchain issues hit so far,
  including a CGB palette bug that was present in the template's
  original `main.c` and is now fixed here.
- See `docs/SETUP.md` for how to wire up save data or music later if
  this project ever needs them.
