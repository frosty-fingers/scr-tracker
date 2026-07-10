#include <gbdk/platform.h>
#include <gb/gb.h>
#include <gb/cgb.h>

// MINIMAL DIAGNOSTIC BUILD - see docs/STATUS.md.
//
// No stdio.h / printf / gotoxy anywhere in this file - the goal is to
// find out whether a hand-placed custom background tile shows up when
// the font/console system is completely out of the picture. The real
// UI (main.c) is backed up outside this file and will be restored once
// this test gives a clear answer.
//
// If a solid black square shows up at the center of the screen, custom
// tile placement works fine on its own - the font/console system is
// somehow interfering with it in the full version. If nothing shows up
// here either, the problem isn't about stdio/printf at all.

#define TILE_ID   1u
#define TILE_COL  10u
#define TILE_ROW  9u

void main(void) {
    static const uint8_t solid_tile[16] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    };
    static const uint8_t tile_id = TILE_ID;

    if (_cpu == CGB_TYPE) {
        set_default_palette();
    }

    set_bkg_data(TILE_ID, 1u, solid_tile);
    set_bkg_tiles(TILE_COL, TILE_ROW, 1u, 1u, &tile_id);

    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        vsync();
    }
}
