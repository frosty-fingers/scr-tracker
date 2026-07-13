# Gotchas

Confirmed-on-real-device (or real emulator) issues and their fixes. Add to
this as new ones turn up — the point is to never debug the same thing twice.

## CGB palette must be set explicitly, or screen renders blank/white

**Symptom:** ROM builds and runs (no crash), but the screen shows solid
white — no visible sprites, background, or text — on Delta (iOS) and
presumably other CGB-aware emulators/hardware.

**Cause:** Building with `-Wm-yc` (CGB-compatible mode) only sets a header
flag; it doesn't configure a color palette. Left unset, the hardware/emulator
can end up rendering foreground and background as the same color —
effectively invisible — rather than falling back to a sane default.

**Fix:** Force the classic grayscale palette explicitly at startup, before
enabling the display:

\`\`\`c
#include <gb/cgb.h>

if (_cpu == CGB_TYPE) {
    set_default_palette();
}
\`\`\`

**Dead end that looked plausible but wasn't:** targeting the platform with
`-msm83:gbc` in LCCFLAGS. GBDK's Game Boy family only has one platform
(`gb`) — color mode is controlled purely by the `-Wm-yc`/`-Wm-yC` header
flags, not by `-m`. Passing `gbc` as a platform name fails at compile time
with `unrecognized PORT:PLATFORM`.

**Confirmed:** 2026-07-09, GBDK-2020 4.5.0, tested on Delta (iOS), via the
CI build in `.github/workflows/build-rom.yml`.

## music.h missing stdint.h caused a syntax error in music.c

**Symptom:** `make` failed compiling `src/music.c` with a syntax error
pointing at a parameter name in `music.h`, even though `main.c` (which
also includes `music.h`) compiled fine.

**Cause:** `music.h` used `uint8_t` without including `<stdint.h>` itself.
It happened to work in `main.c` because other headers pulled in
`<stdint.h>` first — but `music.c` includes `"music.h"` before
`<stdint.h>`, so `uint8_t` was undefined at that point.

**Fix:** Every header should include its own dependencies rather than
relying on include order. Added `#include <stdint.h>` to the top of
`music.h` (already applied in this repo).

**Confirmed:** 2026-07-09, GBDK-2020 4.5.0, via CI build.

## tools/release.sh needs to be run via `bash`, not directly

**Symptom:** `make release` failed with `Permission denied` on
`./tools/release.sh` in CI, right after a successful build.

**Cause:** The file was created through GitHub's web editor, which
doesn't preserve the executable bit — there's no way to `chmod +x` a
file from that UI, only from a real git client.

**Fix:** Call it as `bash tools/release.sh` in the Makefile instead of
`./tools/release.sh`, so it doesn't depend on the executable bit at all
(already applied in this repo's Makefile).

**Confirmed:** 2026-07-09, via CI build.

## gotoxy()/printf() text needs `<gbdk/console.h>`, not just `<stdio.h>`

**Symptom:** `make` failed compiling `src/main.c` with `warning 112:
function 'gotoxy' implicit declaration` followed by `error 101: too many
parameters` on every `gotoxy()` call site. `printf()` calls on their own
compiled fine.

**Cause:** `printf()`'s font/text rendering comes from `<stdio.h>`, but
`gotoxy()` (and `posx()`/`posy()`/`setchar()`/`cls()`) are declared in a
separate header, `<gbdk/console.h>`. Without a real declaration in scope,
the compiler falls back to an implicit one that doesn't know the real
parameter count, so it rejects the call outright once it later sees the
actual two-argument definition.

**Fix:** `#include <gbdk/console.h>` alongside `<stdio.h>` any time
`gotoxy()` is used.

**Confirmed:** 2026-07-09, GBDK-2020 4.5.0, via CI build.

## printf("%u", ...) with a uint8_t prints garbage (values off by multiples of 256)

**Symptom:** Builds and runs fine, no compiler errors, but numbers on
screen are nonsense - e.g. a counter that should read `0` shows `256`,
`512`, `768` (each one 256 apart), and a value that should be `20` shows
something like `23246`.

**Cause:** In standard C, a `char`/`uint8_t` argument passed to a varargs
function like `printf()` is automatically promoted to a full `int`.
SDCC does **not** do this by default - the byte stays exactly 1 byte on
the stack. `%u` always reads a full 2-byte `unsigned int` off the stack
(GB `int` is 16-bit), so with only 1 byte actually pushed, it reads one
extra garbage byte from whatever's next on the stack as the high byte.
Casting the value to `(uint8_t)` before passing it (as this project
originally did) doesn't help - it's already a uint8_t, so the cast is a
no-op and the bug remains.

**Fix:** Cast to a type that's actually the same width `%u`/`%d` expect:

\`\`\`c
printf("%u", (unsigned int)my_uint8_value);
\`\`\`

This is called out directly in GBDK's own coding guidelines - varargs
arguments must be explicitly cast to match, since SDCC skips the usual
C integer promotion.

**Confirmed:** 2026-07-09, GBDK-2020 4.5.0, tested on Delta (iOS).

## Custom background tiles placed before the first printf()/gotoxy() get wiped out

**Symptom:** Custom background tiles (icons, sprites-as-background,
anything placed with `set_bkg_data()`/`set_bkg_tiles()`) don't show up
at all - the screen shows solid background wherever they should be -
even though the exact same placement code worked fine elsewhere, and
regular text/printf output renders correctly on the same screen. No
compiler warnings or errors; this is a silent runtime issue.

Confirmed via a minimal test: a single hand-placed solid tile, with NO
`#include <stdio.h>` and no `printf`/`gotoxy` anywhere in the program,
rendered correctly. Adding the normal UI (with `printf`/`gotoxy` calls
happening *after* the tile placement) made the exact same tile placement
code stop working.

**Cause:** The font/console system does some kind of one-time
initialization the first time `printf()` or `gotoxy()` is used, which
clears the background tile map. If any custom tiles were placed via
`set_bkg_tiles()` *before* that first call, they get overwritten by
that clear - even if the clear itself isn't visible yet (display still
off) and even if nothing ever explicitly calls `cls()`.

**Fix:** Force that one-time console setup to happen with a throwaway
`gotoxy()`/`printf()` call *before* placing any custom background
tiles, not after:

\`\`\`c
gotoxy(0, 0);
printf(" ");        // "primes" the console/font system once

set_bkg_data(...);   // custom tiles are now safe to place
set_bkg_tiles(...);
\`\`\`

More generally: do all `set_bkg_data()`/`set_bkg_tiles()` calls for
custom (non-text) tiles *after* the first real `printf`/`gotoxy` call
in the program, not before, whenever both are used together. Turning
the display on (`SHOW_BKG`/`DISPLAY_ON`) only needs to happen once,
after everything (text and custom tiles) has been placed.

**Confirmed:** 2026-07-10, GBDK-2020 4.5.0, tested on Delta (iOS).

## set_bkg_palette() into slot 0 recolors all default text

**Symptom:** Called `set_bkg_palette()` to give a custom background
tile (e.g. an icon) its own color, expecting only that tile to change.
Instead, *all* regular text on screen (anything printed via plain
`printf`/`gotoxy` with no custom attribute) changed color too.

**Cause:** Every background tile has a CGB palette *slot* (0-7) applied
via its attribute byte. Tiles that never get an explicit attribute set
(which is every tile the font/console system draws by default) use
slot 0. If `set_bkg_palette()` writes new colors into slot 0 - e.g.
`set_bkg_palette(0, 4, my_palettes)` starting at slot 0 - it overwrites
whatever colors the font was implicitly relying on, recoloring all
default text along with it.

**Fix:** Reserve slot 0 for default text/background and never load
anything else into it. Start custom per-tile palettes at slot 1
instead:

\`\`\`c
set_bkg_palette(0, 5, ui_palettes);   // slot 0 = text, slots 1-4 = elements
...
attr[i] = element_index + 1;          // never just element_index (that's slot 0!)
\`\`\`

**Confirmed:** 2026-07-10, GBDK-2020 4.5.0, tested on Delta (iOS).

## cls() doesn't clear CGB per-tile color attributes, only tile IDs

**Symptom:** After leaving one screen/mode and entering another (e.g.
returning to a title screen from gameplay), random pieces of the new
screen's text show up in the wrong color - colors that don't belong to
anything currently being drawn, but that match a color used on the
*previous* screen at that same row/column position.

**Cause:** `cls()` clears the background tile ID plane (fills it with
the blank tile) but does **not** touch the separate CGB attribute plane
that per-tile `set_bkg_tiles()`-with-`VBK_ATTRIBUTES` calls write to
(see the `set_bkg_palette()`/slot 0 gotcha above for how that plane is
used). Once a cell's attribute byte is set to some non-zero palette
slot, it stays that way indefinitely - across `cls()` calls, across
screen transitions, even across completely different game modes -
until something explicitly overwrites that exact cell's attribute
again. If new text happens to land on a cell a previous screen painted,
it inherits that old color.

**Fix:** Explicitly reset the whole visible screen's attribute plane
back to palette slot 0 (the default text palette) any time transitioning
to a new screen - a bare `cls()` is not enough:

\`\`\`c
static void clear_attributes(void) {
    uint8_t attr[20] = {0};   // all slot 0
    uint8_t y;
    VBK_REG = VBK_ATTRIBUTES;
    for (y = 0; y < 18; y++) {
        set_bkg_tiles(0, y, 20, 1, attr);
    }
    VBK_REG = VBK_TILES;
}
\`\`\`

Call this alongside `cls()` (a combined `full_clear()` helper that does
both) everywhere the game changes screens, not just once at startup.

**Confirmed:** 2026-07-10, GBDK-2020 4.5.0, tested on Delta (iOS).

## A local buffer sized for one call site overflowed on a wider one

**Symptom:** 1-player mode rendered garbage or went blank/white after
adding Death's Door red-coloring; 2-player mode was completely
unaffected by the same change.

**Cause:** `paint_row()`'s internal attribute buffer was declared as
`uint8_t attr[MAX_SYMBOLS]` (8 bytes) - sized for its original use
(coloring a row of up to 8 element symbols). A later change reused the
same function to color the 16-character "DEATHS DOOR" message
(`paint_row(2, ROW, 16, FIRE_PAL)`), writing 16 bytes into that 8-byte
stack buffer - a real stack buffer overflow, corrupting whatever else
was on the stack at the time. 1-player mode broke because it's the one
place that message is 16 characters wide; 2-player mode's equivalent
"DOOR" message is only 4 characters, well within the old buffer, so it
never triggered the overflow.

**Fix:** size shared buffers like this for the *actual* maximum width
they'll ever be called with (here, the full 20-column screen width),
not just whatever the first caller happened to need:

```c
uint8_t attr[20];   // not uint8_t attr[MAX_SYMBOLS] (8) - see above
```

**Lesson**: when reusing a small helper for a new, wider call site,
check its internal buffer sizes - a function working fine for its
original caller doesn't mean it's safe for a bigger one.

**Confirmed:** 2026-07-11, GBDK-2020 4.5.0, tested on Delta (iOS).
