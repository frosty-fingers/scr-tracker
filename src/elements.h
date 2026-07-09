#ifndef ELEMENTS_H
#define ELEMENTS_H

#include <stdint.h>

// Original, simple 8x8 pixel-art icons for the four Sorcery: Contested
// Realm elements (Fire / Water / Earth / Air). These are NOT the game's
// official threshold symbols - those are Sorcery TCG artwork - just
// readable stand-ins so this doesn't depend on any external image asset.
//
// Each tile is 16 bytes: 8 rows x 2 bytes/row (GB 2bpp format). Both
// bitplane bytes are identical per row, so every "on" pixel renders as
// palette color 3 (darkest shade) for max contrast against the background.

#define TILE_FIRST_ELEMENT   128u
#define TILE_FIRE   (TILE_FIRST_ELEMENT + 0u)
#define TILE_WATER  (TILE_FIRST_ELEMENT + 1u)
#define TILE_EARTH  (TILE_FIRST_ELEMENT + 2u)
#define TILE_AIR    (TILE_FIRST_ELEMENT + 3u)

static const uint8_t element_tiles[4u * 16u] = {
    // FIRE - flame
    0x18,0x18, 0x38,0x38, 0x7C,0x7C, 0x7E,0x7E,
    0xFF,0xFF, 0xFF,0xFF, 0x7E,0x7E, 0x3C,0x3C,

    // WATER - droplet
    0x18,0x18, 0x18,0x18, 0x3C,0x3C, 0x7E,0x7E,
    0xFF,0xFF, 0xFF,0xFF, 0x7E,0x7E, 0x3C,0x3C,

    // EARTH - mountain
    0x00,0x00, 0x18,0x18, 0x3C,0x3C, 0x7E,0x7E,
    0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF, 0x00,0x00,

    // AIR - wind swirl
    0x00,0x00, 0xE7,0xE7, 0x00,0x00, 0x73,0x73,
    0x00,0x00, 0x39,0x39, 0x00,0x00, 0xFF,0xFF,
};

#endif // ELEMENTS_H
