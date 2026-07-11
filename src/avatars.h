#ifndef AVATARS_H
#define AVATARS_H

// Player avatar list - this is the file to edit to add/change
// characters. No other files need to change to add a new one.
//
// HOW TO ADD ONE: add a new "XXX" entry below (exactly 3 characters -
// pad short codes with a trailing space, e.g. "FOX") and bump
// AVATAR_COUNT to match. That's it.
//
// This is name/code-only for now - shown in-game as a 3-letter code in
// brackets, e.g. "[WIZ]", picked at the start of a game via LEFT/RIGHT.
// Swapping in real sprite art later (once you have PNGs) would mean:
//   1. Add the PNG to res/gfx/ and convert it with `make assets`
//      (see res/gfx/README.md) into 8x8/16x16 tile data, same way
//      elements.h's icons were hand-built.
//   2. Add a matching tile ID constant here next to each code, e.g.
//      AVATAR_TILE_WIZ, so each entry carries both a code and a tile.
//   3. Wherever main.c currently prints the 3-letter code (the
//      avatar_draw() and header-drawing functions), swap in a
//      set_bkg_tiles() call for that avatar's tile instead.
// The selection logic (cycling with LEFT/RIGHT, confirming with A)
// won't need to change for that - only what gets drawn per choice.

#define AVATAR_COUNT  6u

static const char *avatar_codes[AVATAR_COUNT] = {
    "WIZ",
    "ROG",
    "KNT",
    "DRU",
    "RGR",
    "BRD",
};

#endif // AVATARS_H
