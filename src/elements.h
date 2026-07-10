#ifndef ELEMENTS_H
#define ELEMENTS_H

#include <stdint.h>

// 8x8 (single tile) pixel-art icons for the four classical elements, in
// AIR / EARTH / FIRE / WATER order - alchemical triangle symbols
// (upward = Fire, downward = Water, triangle + bar sticking past its
// edges = Air/Earth). Public-domain symbols, not the game's specific
// artwork - hand-authored here rather than traced from any image.
//
// DIAGNOSTIC NOTE (see docs/STATUS.md): this is intentionally back to
// single 8x8 tiles per icon (not 2x2/16x16) using the exact same
// set_bkg_data()/set_bkg_tiles() placement mechanism that was confirmed
// rendering correctly in the very first version of this project. The
// 16x16 multi-tile version stopped rendering and the cause hasn't been
// found yet - this isolates whether the bug is in multi-tile placement
// specifically before spending more time on it.
//
// Pixel values: 0 = background (matches the screen's white), 3 = outline
// only (hollow interior, no fill color).

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

#endif // ELEMENTS_H
