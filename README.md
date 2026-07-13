# Sorcery: Contested Realm - Life & Threshold Tracker

A Game Boy Color homebrew ROM (built on the GBDK-2020 project template)
for tracking Life (20 -> Death's Door at 0) and the four elemental
thresholds (Fire, Water, Earth, Air) during a game of *Sorcery:
Contested Realm*.

No save data - this is meant to be reset fresh each game. A short
placeholder sound plays on +/-; no music yet.

## Title screen and avatar select

On boot: "1 PLAYER" / "2 PLAYER" / "SETTINGS". UP/DOWN to choose, A or
START to confirm. Choosing SETTINGS opens a theme picker (LEFT/RIGHT
to cycle, changes apply live; B or START to go back) - see "Themes"
below. Choosing 1 PLAYER goes straight into play (it's obviously just
you - no avatar select needed). Choosing 2 PLAYER moves on to picking
an avatar (LEFT/RIGHT to cycle, A/START to confirm) for P1, then P2.
The select screen shows each avatar's full name; the in-game header
shows a short 3-letter code instead since there isn't room there for a
full name. See `src/avatars.h` to add or change avatars; real sprite
art is a planned future step, not done yet.

From anywhere in gameplay, holding START+SELECT together returns to
the title screen.

## Controls (1 player)

| Button      | Action                                                    |
|-------------|-------------------------------------------------------------|
| UP/DOWN     | Move the cursor between LIFE and the elements (AIR/EARTH/FIRE/WATER) |
| A           | +1 to the selected counter (hold to auto-repeat)            |
| B           | -1 to the selected counter (hold to auto-repeat)            |
| START       | Reset (LIFE to 20, elements to 0)                            |

## Controls (2 player)

Screen splits into a left half (P1) and a right half (P2), mirrored -
P2's layout is a mirror image of P1's, with symbols growing toward the
middle of the screen from each side.

There's only one set of buttons, so control is turn-based: SELECT
swaps which player UP/DOWN/A/B/START currently affect (shown via
`[XXX]` around the focused player's avatar code in the header). START
only resets the *currently focused* player, not both. The cursor arrow
points toward the middle of the screen on each side (right for P1,
left for P2), matching the mirrored layout.

LIFE locks at Death's Door (0) for whichever player hits it - A/B do
nothing for that player until START resets them.

## Symbols

Each element's count is shown as that many repeated element symbols in
a row (up to 8 in 1-player mode) rather than a number - if it goes
over 8, the exact total is printed alongside. (2-player mode also caps
at 8 symbols per row, but doesn't have room to show the exact total if
it goes over - a known, accepted limitation given the smaller layout.)

## Glossary

From the title screen ("GLOSSARY") or, during a match, hold SELECT and
press DOWN. Shows a scrollable list of terms - UP/DOWN to move, A to
view a definition, B to go back (from a definition, back to the list;
from the list, back to wherever you opened it from). Content lives in
`src/glossary.h` - it currently has placeholder example entries, not
real rules text. See the comment at the top of that file for the
format (term + up to 6 pre-wrapped lines of definition per entry) and
size limits.

## Dice / coin roller

During a match (either mode), hold SELECT and press UP to open it -
every other button was already in use, so this was the one combo left.
LEFT/RIGHT picks D20, D6, or a coin flip; A rolls; B goes back to the
game exactly as you left it. Random results are seeded from the
hardware's free-running timer on every roll, not just once at startup.

## Themes

On CGB hardware, pick a color theme from the title screen's SETTINGS:

| Theme     | Background | Text  | AIR        | EARTH | FIRE       | WATER      |
|-----------|------------|-------|------------|-------|------------|------------|
| GREY      | grey       | gold  | light grey | brown | bright red | bright blue |
| PARCHMENT | warm tan   | dark ink | near white | dark brown | red | blue |
| MIDNIGHT  | dark navy  | pale  | grey       | amber brown | orange-red | cyan blue |

Falls back to the hardware's default grayscale on original DMG (themes
have no effect there). Theme choice resets on power-on - there's no
save data.

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
- **`src/save.c`/`src/save.h` and `src/music.c`/`src/music.h`** are
  still here from the template but unused/uncalled - left in case a
  future version wants persistence. Background music is intentionally
  on hold (see docs/STATUS.md) - being composed separately.
- **`src/avatars.h`** is the editable list of player avatars (full
  name + short in-game code per entry) - see the comment at the top of
  that file for how to add more, and the planned path to real sprite
  art later.
- **Element icons are original 8x8 pixel art** (classical alchemical
  triangle symbols for Air/Earth/Fire/Water - hollow outline, bar
  sticking past the edges for Air/Earth) in `src/elements.h` - not the
  game's official threshold symbols, which are Sorcery TCG's
  copyrighted artwork. Colored on CGB (see "Symbols and color" above),
  grayscale on DMG.
- See `docs/GOTCHAS.md` for hardware/toolchain issues hit so far,
  including a CGB palette bug that was present in the template's
  original `main.c` and is now fixed here.
- See `docs/SETUP.md` for how to wire up save data or music later if
  this project ever needs them.
