#ifndef AVATARS_H
#define AVATARS_H

// Player avatar list - this is the file to edit to add/change
// characters. No other files need to change to add a new one.
//
// Two parallel arrays per avatar, kept in the same order:
//   avatar_names[] - full display name, shown on the avatar SELECT
//                     screen (plenty of room there - up to
//                     AVATAR_NAME_MAXLEN characters, currently 18).
//                     Longer names will overflow past the right arrow,
//                     so keep to the limit.
//   avatar_codes[] - a short 3-character code, shown in the compact
//                     in-game 2-player header (e.g. "[WIZ]") - there's
//                     only room for ~5 characters total there,
//                     including the brackets, so full names don't fit
//                     in-game. Pad short codes with a trailing space.
//
// Placeholder names below are original, not the game's official avatar
// names - swap in the real ones here if you want them; nothing else in
// the code needs to change either way.
//
// HOW TO ADD ONE: add a name to avatar_names, a matching code to
// avatar_codes (same index), and bump AVATAR_COUNT.
//
// Swapping in real sprite art later (once you have PNGs) would mean:
//   1. Add the PNG to res/gfx/ and convert it with `make assets`
//      (see res/gfx/README.md) into tile data, same way elements.h's
//      icons were hand-built.
//   2. Add a matching tile ID constant here next to each entry.
//   3. Wherever main.c currently prints a name/code, swap in a
//      set_bkg_tiles() call for that avatar's tile instead.
// The selection logic (cycling with LEFT/RIGHT, confirming with A)
// won't need to change for that - only what gets drawn.

#define AVATAR_COUNT        6u
#define AVATAR_NAME_MAXLEN  18u

static const char *avatar_names[AVATAR_COUNT] = {
    "Wanderer",
    "Trickster",
    "Sentinel",
    "Voyager",
    "Outlander",
    "Pathfinder",
};

static const char *avatar_codes[AVATAR_COUNT] = {
    "WND",
    "TRK",
    "SNT",
    "VOY",
    "OUT",
    "PTH",
};

#endif // AVATARS_H
