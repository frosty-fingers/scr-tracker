#ifndef ELEMENTS_H
#define ELEMENTS_H

#include <stdint.h>
#include <gb/cgb.h>

// 8x8 (single tile) pixel-art icons for the four classical elements, in
// AIR / EARTH / FIRE / WATER order - alchemical triangle symbols
// (upward = Fire, downward = Water, triangle + bar sticking past its
// edges = Air/Earth). Public-domain symbols, not the game's specific
// artwork - hand-authored here rather than traced from any image.
//
// Pixel values: 0 = background, 3 = outline only (hollow interior, no
// separate fill color). See docs/GOTCHAS.md for why any custom tile
// placement must happen after the console/font system's first use.

#define TILE_FIRST_ELEMENT   128u

#define TILE_AIR    (TILE_FIRST_ELEMENT + 0u)
#define TILE_EARTH  (TILE_FIRST_ELEMENT + 1u)
#define TILE_FIRE   (TILE_FIRST_ELEMENT + 2u)
#define TILE_WATER  (TILE_FIRST_ELEMENT + 3u)

static const uint8_t element_tiles[4u * 16u] = {
    // AIR
    0x18,0x18,0x18,0x18,0x24,0x24,0x24,0x24,0x42,0x42,0xFF,0xFF,0x81,0x81,0xFF,0xFF,
    // EARTH
    0xFF,0xFF,0x81,0x81,0xFF,0xFF,0x42,0x42,0x24,0x24,0x24,0x24,0x18,0x18,0x18,0x18,
    // FIRE
    0x18,0x18,0x18,0x18,0x24,0x24,0x24,0x24,0x42,0x42,0x42,0x42,0x81,0x81,0xFF,0xFF,
    // WATER
    0xFF,0xFF,0x81,0x81,0x42,0x42,0x42,0x42,0x24,0x24,0x24,0x24,0x18,0x18,0x18,0x18,
};

// All 5 CGB background palettes used in the game, loaded together in
// one set_bkg_palette() call starting at slot 0:
//   slot 0        - default text/UI (used by every printf/gotoxy call
//                   that doesn't get a custom attribute) - grey
//                   background, gold ink.
//   slots 1-4     - one per element (AIR/EARTH/FIRE/WATER), applied via
//                   per-tile attributes in paint_row() (main.c). Same
//                   grey background as slot 0 so icons blend in
//                   seamlessly; only color 3 differs (each element's
//                   own outline color), since the icons are hollow and
//                   never use color 1 or 2.
//
// IMPORTANT: slot 0 must never be reused for an element palette (see
// docs/GOTCHAS.md - that was the bug that made all text turn yellow).
// Elements start at slot 1, not slot 0.
//
// Background is a neutral grey rather than brown specifically so it
// doesn't compete with EARTH's brown - see docs/GOTCHAS.md for the
// separate (now fixed) bug where leftover per-tile colors from a
// previous screen could bleed through if this ever looked wrong after
// a screen transition.
#define BG_R  11
#define BG_G  11
#define BG_B  13

static const palette_color_t ui_palettes[5u * 4u] = {
    // slot 0 - default text: grey background, gold ink
    RGB(BG_R,BG_G,BG_B), RGB(BG_R,BG_G,BG_B), RGB(BG_R,BG_G,BG_B), RGB(28,22,6),
    // slot 1 - AIR: light grey (kept distinct from both the background and the gold text)
    RGB(BG_R,BG_G,BG_B), RGB(BG_R,BG_G,BG_B), RGB(BG_R,BG_G,BG_B), RGB(23,23,25),
    // slot 2 - EARTH: brown
    RGB(BG_R,BG_G,BG_B), RGB(BG_R,BG_G,BG_B), RGB(BG_R,BG_G,BG_B), RGB(18,11,4),
    // slot 3 - FIRE: red
    RGB(BG_R,BG_G,BG_B), RGB(BG_R,BG_G,BG_B), RGB(BG_R,BG_G,BG_B), RGB(24,4,4),
    // slot 4 - WATER: blue
    RGB(BG_R,BG_G,BG_B), RGB(BG_R,BG_G,BG_B), RGB(BG_R,BG_G,BG_B), RGB(4,10,24),
};

#endif // ELEMENTS_H
