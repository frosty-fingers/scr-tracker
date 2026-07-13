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

// Color themes - each one is a full set of 6 CGB background palettes
// (slot 0 = default text/UI, slots 1-4 = AIR/EARTH/FIRE/WATER, slot 5
// = accent - used for selector indicators like the ">"/"<" cursors and
// "[" "]" brackets, so they visually pop as a highlight rather than
// blending into the regular text color), picked from the title
// screen's SETTINGS option and applied live via apply_theme() in
// main.c. Adding a theme: add a name to theme_names and a matching
// 24-entry palette block to theme_palettes (same slot layout as
// above), then bump THEME_COUNT. Selection is session-only (resets on
// power-on) since this project has no save data.
//
// IMPORTANT: slot 0 must never be reused for an element palette (see
// docs/GOTCHAS.md - that was the bug that made all text turn yellow).
// Elements start at slot 1, not slot 0, in every theme.
#define THEME_COUNT  3u

static const char *theme_names[THEME_COUNT] = {
    "GREY",
    "PARCHMENT",
    "MIDNIGHT",
};

static const palette_color_t theme_palettes[THEME_COUNT][6u * 4u] = {
    // GREY - grey background, gold text
    {
        RGB(11,11,13), RGB(11,11,13), RGB(11,11,13), RGB(28,22,6),   // text
        RGB(11,11,13), RGB(11,11,13), RGB(11,11,13), RGB(23,23,25),  // AIR - light grey
        RGB(11,11,13), RGB(11,11,13), RGB(11,11,13), RGB(26,16,5),   // EARTH - brown
        RGB(11,11,13), RGB(11,11,13), RGB(11,11,13), RGB(31,6,6),    // FIRE - bright red
        RGB(11,11,13), RGB(11,11,13), RGB(11,11,13), RGB(6,14,31),   // WATER - bright blue
        RGB(11,11,13), RGB(11,11,13), RGB(11,11,13), RGB(8,28,30),   // ACCENT - bright cyan
    },
    // PARCHMENT - warm tan background, dark ink text
    {
        RGB(20,16,9), RGB(20,16,9), RGB(20,16,9), RGB(6,4,2),        // text
        RGB(20,16,9), RGB(20,16,9), RGB(20,16,9), RGB(30,30,30),     // AIR - near white
        RGB(20,16,9), RGB(20,16,9), RGB(20,16,9), RGB(14,8,2),       // EARTH - dark brown
        RGB(20,16,9), RGB(20,16,9), RGB(20,16,9), RGB(26,4,2),       // FIRE - red
        RGB(20,16,9), RGB(20,16,9), RGB(20,16,9), RGB(2,10,26),      // WATER - blue
        RGB(20,16,9), RGB(20,16,9), RGB(20,16,9), RGB(20,2,6),       // ACCENT - deep crimson
    },
    // MIDNIGHT - dark navy background, pale text
    {
        RGB(2,3,9), RGB(2,3,9), RGB(2,3,9), RGB(24,24,28),           // text
        RGB(2,3,9), RGB(2,3,9), RGB(2,3,9), RGB(18,18,20),           // AIR - grey
        RGB(2,3,9), RGB(2,3,9), RGB(2,3,9), RGB(20,13,4),            // EARTH - amber brown
        RGB(2,3,9), RGB(2,3,9), RGB(2,3,9), RGB(31,10,6),            // FIRE - orange-red
        RGB(2,3,9), RGB(2,3,9), RGB(2,3,9), RGB(6,18,31),            // WATER - cyan blue
        RGB(2,3,9), RGB(2,3,9), RGB(2,3,9), RGB(31,22,4),            // ACCENT - bright gold
    },
};

#endif // ELEMENTS_H
