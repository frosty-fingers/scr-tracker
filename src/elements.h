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
// Pixel values: 0 = background (white), 3 = outline only (hollow
// interior, no separate fill color). See docs/GOTCHAS.md for why any
// custom tile placement must happen after the console/font system's
// first use, not before.

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

// Standard elemental colors (CGB only - DMG hardware just gets the
// default grayscale palette). One palette per element, applied via
// per-tile BG attributes (see paint_row() in main.c). Since the icons
// are hollow (only pixel indices 0 and 3 are ever used - no separate
// fill color), only color slot 3 (the outline) actually matters;
// slots 0-2 are set to white to match the rest of the screen in case
// they're ever sampled. Palette slot order matches element order:
// 0 = AIR, 1 = EARTH, 2 = FIRE, 3 = WATER.
static const palette_color_t element_palettes[4u * 4u] = {
    // AIR - yellow
    RGB(31,31,31), RGB(31,31,31), RGB(31,31,31), RGB(30,28,4),
    // EARTH - green
    RGB(31,31,31), RGB(31,31,31), RGB(31,31,31), RGB(4,22,4),
    // FIRE - red
    RGB(31,31,31), RGB(31,31,31), RGB(31,31,31), RGB(28,4,4),
    // WATER - blue
    RGB(31,31,31), RGB(31,31,31), RGB(31,31,31), RGB(4,12,28),
};

#endif // ELEMENTS_H
