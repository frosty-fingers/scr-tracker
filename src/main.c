#include <gbdk/platform.h>
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdio.h>
#include <stdint.h>
#include <gbdk/console.h>

#include "elements.h"

// NOTE: this project has no save data and no music, so save.h/music.h
// (both still present in src/) are intentionally not included here.

#define LIFE_START   20u
#define LIFE_MIN     0u
#define LIFE_MAX     20u
#define ELEM_MIN     0u
#define ELEM_MAX     99u

#define COUNTER_LIFE  0u
#define COUNTER_AIR   1u
#define COUNTER_EARTH 2u
#define COUNTER_FIRE  3u
#define COUNTER_WATER 4u
#define COUNTER_COUNT 5u

// Order requested: AIR, EARTH, FIRE, WATER - laid out as a 2x2 grid.
// icon_col/icon_row are the tile position of each 8x8 icon; label/value
// text sits 2 columns to the right of the icon.
static const uint8_t icon_col[4]   = { 1u, 11u, 1u, 11u };
static const uint8_t icon_row[4]   = { 7u, 7u, 11u, 11u };
static const uint8_t icon_tile[4]  = { TILE_AIR, TILE_EARTH, TILE_FIRE, TILE_WATER };
static const char *element_label[4] = { "AIR", "EARTH", "FIRE", "WATER" };

#define LABEL_COL_OFFSET  2u
#define CURSOR_COL_OFFSET 1u

#define ROW_TITLE1    0u
#define ROW_TITLE2    1u
#define ROW_LIFE_CUR  3u
#define ROW_LIFE      3u
#define ROW_DEATHDOOR 5u
#define ROW_HINT1     15u
#define ROW_HINT2     16u

// How long (in frames, ~60/sec) A/B must be held before auto-repeat
// kicks in, and how many frames between each repeated step after that.
#define REPEAT_DELAY     18u
#define REPEAT_INTERVAL  5u

static uint8_t counters[COUNTER_COUNT];
static uint8_t active = COUNTER_LIFE;

// Sets up the CGB grayscale-compatible palette (see docs/GOTCHAS.md -
// required or the screen can render blank/white on CGB hardware), loads
// the element icon tiles, and turns the display on.
//
// NOTE: single-tile placement is intentional right now - see the note
// at the top of elements.h and docs/STATUS.md.
static void init_graphics(void) {
    uint8_t i;

    if (_cpu == CGB_TYPE) {
        set_default_palette();
    }

    set_bkg_data(TILE_FIRST_ELEMENT, 4u, element_tiles);

    for (i = 0u; i < 4u; i++) {
        set_bkg_tiles(icon_col[i], icon_row[i], 1u, 1u, &icon_tile[i]);
    }

    SHOW_BKG;
    DISPLAY_ON;
}

static void reset_counters(void) {
    counters[COUNTER_LIFE] = LIFE_START;
    counters[COUNTER_AIR] = 0u;
    counters[COUNTER_EARTH] = 0u;
    counters[COUNTER_FIRE] = 0u;
    counters[COUNTER_WATER] = 0u;
}

// Prints a counter value left-aligned followed by a blanking space, so
// a value that shrinks by a digit (e.g. 10 -> 9) doesn't leave a stale
// digit behind. GBDK's printf doesn't reliably honor width specifiers
// like "%2u" (see docs/GOTCHAS.md), so we don't rely on one here.
static void print_value(uint8_t value) {
    printf("%u ", (unsigned int)value);
}

// Draws everything that never changes: title, icons, element labels.
static void draw_static_ui(void) {
    uint8_t i;

    gotoxy(6u, ROW_TITLE1);
    printf("SORCERY");
    gotoxy(4u, ROW_TITLE2);
    printf("LIFE TRACKER");

    gotoxy(3u, ROW_LIFE);
    printf("LIFE");

    for (i = 0u; i < 4u; i++) {
        gotoxy(icon_col[i] + LABEL_COL_OFFSET, icon_row[i]);
        printf(element_label[i]);
    }

    gotoxy(0u, ROW_HINT1);
    printf("<>SEL A:+ B:-");
    gotoxy(0u, ROW_HINT2);
    printf("START:RESET");
}

// Redraws the LIFE value and its cursor marker. Only called when LIFE's
// value changed or the cursor moved on/off it, so there's no per-frame
// flicker from constantly reprinting text.
static void draw_life(void) {
    gotoxy(0u, ROW_LIFE_CUR);
    printf((active == COUNTER_LIFE) ? ">" : " ");

    gotoxy(9u, ROW_LIFE);
    print_value(counters[COUNTER_LIFE]);

    gotoxy(2u, ROW_DEATHDOOR);
    printf((counters[COUNTER_LIFE] == LIFE_MIN) ? "  DEATHS DOOR   " : "                ");
}

// Redraws all 4 element values and whichever cursor arrow is active.
static void draw_elements(void) {
    uint8_t i;

    for (i = 0u; i < 4u; i++) {
        gotoxy(icon_col[i] + LABEL_COL_OFFSET, icon_row[i] + 1u);
        print_value(counters[COUNTER_AIR + i]);

        gotoxy(icon_col[i] + CURSOR_COL_OFFSET, icon_row[i]);
        printf((active == COUNTER_AIR + i) ? ">" : " ");
    }
}

// Returns 1 if the active counter is currently allowed to change, 0 if
// it's locked. LIFE locks at Death's Door (0) until START resets it -
// elements are never locked.
static uint8_t counter_is_locked(void) {
    return (active == COUNTER_LIFE) && (counters[COUNTER_LIFE] == LIFE_MIN);
}

static void apply_delta(int8_t delta) {
    uint8_t value;
    uint8_t min;
    uint8_t max;

    if (counter_is_locked()) {
        return;
    }

    value = counters[active];
    min = (active == COUNTER_LIFE) ? LIFE_MIN : ELEM_MIN;
    max = (active == COUNTER_LIFE) ? LIFE_MAX : ELEM_MAX;

    if (delta > 0) {
        if (value < max) {
            value++;
        }
    } else {
        if (value > min) {
            value--;
        }
    }

    counters[active] = value;
}

static void redraw_active(void) {
    if (active == COUNTER_LIFE) {
        draw_life();
    } else {
        draw_elements();
    }
}

void main(void) {
    uint8_t prev_keys = 0u;
    uint8_t keys, pressed;
    uint8_t a_hold = 0u, b_hold = 0u;

    init_graphics();
    reset_counters();
    draw_static_ui();
    draw_life();
    draw_elements();

    while (1) {
        keys = joypad();
        pressed = keys & (uint8_t)(~prev_keys);
        prev_keys = keys;

        if (pressed & J_LEFT) {
            active = (active == COUNTER_LIFE) ? (COUNTER_COUNT - 1u) : (active - 1u);
            draw_life();
            draw_elements();
        } else if (pressed & J_RIGHT) {
            active = (active == COUNTER_COUNT - 1u) ? COUNTER_LIFE : (active + 1u);
            draw_life();
            draw_elements();
        } else if (pressed & J_START) {
            reset_counters();
            draw_life();
            draw_elements();
        }

        // A: +1 on press, then auto-repeat while held.
        if (keys & J_A) {
            if (pressed & J_A) {
                apply_delta(1);
                redraw_active();
                a_hold = 0u;
            } else {
                a_hold++;
                if (a_hold >= REPEAT_DELAY &&
                    ((a_hold - REPEAT_DELAY) % REPEAT_INTERVAL) == 0u) {
                    apply_delta(1);
                    redraw_active();
                }
            }
        } else {
            a_hold = 0u;
        }

        // B: -1 on press, then auto-repeat while held.
        if (keys & J_B) {
            if (pressed & J_B) {
                apply_delta(-1);
                redraw_active();
                b_hold = 0u;
            } else {
                b_hold++;
                if (b_hold >= REPEAT_DELAY &&
                    ((b_hold - REPEAT_DELAY) % REPEAT_INTERVAL) == 0u) {
                    apply_delta(-1);
                    redraw_active();
                }
            }
        } else {
            b_hold = 0u;
        }

        vsync();
    }
}
