# Current status

_Last updated: 2026-07-11. This file gets rewritten to reflect current
state, not appended to — check git history if you want the old version._

## Current focus

Fixed a real, confirmed bug: 1-player mode went blank/white after last
round's Death's Door red-coloring change, while 2-player was fine.
Root cause found and fixed - see docs/GOTCHAS.md. NOT YET re-tested.

## Confirmed working (via screenshots, before this bug)

- Title screen (4 options), avatar select (2P), 1P mode (before this
  bug), 2P split/mirrored layout - confirmed unaffected by this bug -
  SELECT-swap focus, START+SELECT-to-title, dice roller, glossary.
- UP/DOWN navigation - not yet screenshot-confirmed.
- Theme system - still pending its own confirmation from a few rounds
  back.

## Fixed this round

**Stack buffer overflow in `paint_row()` - confirmed root cause of
"1-player goes white, 2-player is fine".** `paint_row()`'s internal
`attr[]` buffer was sized `MAX_SYMBOLS` (8 bytes) - fine for its
original use (coloring up to 8 element symbols in a row). Last round's
Death's Door coloring reused the same function with a width of 16 (the
"DEATHS DOOR" message) in 1-player mode specifically -
`paint_row(2, ROW, 16, FIRE_PAL)` - writing 16 bytes into an 8-byte
stack array. 2-player's equivalent "DOOR" message is only 4 characters
wide, so it never touched the overflow; that's exactly why the bug was
1-player-only.

**Fix**: `attr[]` is now sized 20 (the actual full screen width, the
real upper bound for anything this function could reasonably be asked
to color), not `MAX_SYMBOLS`. Checked every other `paint_row()` call
site's width argument (1, 4, 8, 16) - all comfortably under 20 now.

Full writeup in docs/GOTCHAS.md, including the general lesson (a
buffer sized for one caller isn't automatically safe for a new, wider
one reusing the same helper).

## Not yet tested

- **1-player mode specifically** - the exact scenario that broke:
  entering 1-player mode, and separately, hitting Death's Door in
  1-player mode (the code path that actually overflowed).
- Everything from the last couple of rounds that was already flagged
  as pending: accent selector colors across all 3 themes, DOOR/DEATHS
  DOOR turning red and back, UP/DOWN navigation, the theme system
  generally.

## Deferred / explicitly out of scope right now

- **Real glossary content**: still placeholder only.
- **Background music**: on hold, a friend is composing chiptune
  separately. Integration will need hUGEDriver vendored in per
  docs/SETUP.md when that's ready.
- **Real avatar sprite art**: still name/code-only by design.
- **2-player overflow indicator**: still no room budgeted for showing
  the exact count when an element goes over 8 symbols in 2-player mode.
- **Theme choice doesn't persist** across power-off (no save data).
