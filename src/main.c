#include <gbdk/platform.h>
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdio.h>
#include <stdint.h>
#include <gbdk/console.h>

#include "elements.h"

// NOTE: this project has no save data and no music, so save.h/music.h
// (both still present in src/) are intentionally not included here.
// See CLAUDE.md / docs/GOTCHAS.md for why the CGB palette call below is
// required - the template's original main.c was missing it, which is
// exactly the "blank white screen" bug that gotcha documents.

#define LIFE_START   20u
#define LIFE_MIN     0u
#define LIFE_MAX     20u
#define ELEM_MIN     0u
#define ELEM_MAX     99u

#define COUNTER_LIFE  0u
#define COUNTER_FIRE  1u
#define COUNTER_WATER 2u
#define COUNTER_EARTH 3u
#define COUNTER_AIR   4u
#define COUNTER_COUNT 5u

// Screen columns where each element's icon/value/cursor live.
static const uint8_t element_col[4] = { 1u, 6u, 11u, 16u };
static const uint8_t element_tile[4] = { TILE_FIRE, TILE_WATER, TILE_EARTH, TILE_AIR };
static const char *element_letter[4] = { "F", "W", "E", "A" };

#define ROW_TITLE1    0u
#define ROW_TITLE2    1u
#define ROW_LIFE_CUR  3u
#define ROW_LIFE      3u
#define ROW_DEATHDOOR 5u
#define ROW_ELEM_ICON 9u
#define ROW_ELEM_VAL  10u
#define ROW_ELEM_CUR  11u
#define ROW_HINT1     15u
#define ROW_HINT2     16u

static uint8_t counters[COUNTER_COUNT];
static uint8_t active = COUNTER_LIFE;

// Sets up the CGB palette (see docs/GOTCHAS.md - required or the screen
// can render blank/white on CGB hardware), loads the element icon
// tiles, and turns the display on.
static void init_graphics(void) {
    if (_cpu == CGB_TYPE) {
        set_default_palette();
    }

    set_bkg_data(TILE_FIRST_ELEMENT, 4u, element_tiles);

    SHOW_BKG;
    DISPLAY_ON;
}

static void reset_counters(void) {
    counters[COUNTER_LIFE] = LIFE_START;
    counters[COUNTER_FIRE] = 0u;
    counters[COUNTER_WATER] = 0u;
    counters[COUNTER_EARTH] = 0u;
    counters[COUNTER_AIR] = 0u;
}

// Draws everything that never changes: title, icons, static labels.
static void draw_static_ui(void) {
    uint8_t i;

    gotoxy(6u, ROW_TITLE1);
    printf("SORCERY");
    gotoxy(4u, ROW_TITLE2);
    printf("LIFE TRACKER");

    gotoxy(3u, ROW_LIFE);
    printf("LIFE");

    for (i = 0u; i < 4u; i++) {
        set_bkg_tiles(element_col[i], ROW_ELEM_ICON, 1u, 1u, &element_tile[i]);
        gotoxy(element_col[i] + 1u, ROW_ELEM_ICON);
        printf(element_letter[i]);
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
    printf("%2u", (unsigned int)counters[COUNTER_LIFE]);

    gotoxy(2u, ROW_DEATHDOOR);
    printf((counters[COUNTER_LIFE] == LIFE_MIN) ? "  DEATHS DOOR   " : "                ");
}

// Redraws all 4 element values and whichever cursor arrow is active.
static void draw_elements(void) {
    uint8_t i;

    for (i = 0u; i < 4u; i++) {
        gotoxy(element_col[i], ROW_ELEM_VAL);
        printf("%2u", (unsigned int)counters[COUNTER_FIRE + i]);

        gotoxy(element_col[i], ROW_ELEM_CUR);
        printf((active == COUNTER_FIRE + i) ? "^^" : "  ");
    }
}

static void apply_delta(int8_t delta) {
    uint8_t value = counters[active];
    uint8_t min = (active == COUNTER_LIFE) ? LIFE_MIN : ELEM_MIN;
    uint8_t max = (active == COUNTER_LIFE) ? LIFE_MAX : ELEM_MAX;

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

void main(void) {
    uint8_t prev_keys = 0u;
    uint8_t keys, pressed;

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
        } else if (pressed & J_A) {
            apply_delta(1);
            if (active == COUNTER_LIFE) {
                draw_life();
            } else {
                draw_elements();
            }
        } else if (pressed & J_B) {
            apply_delta(-1);
            if (active == COUNTER_LIFE) {
                draw_life();
            } else {
                draw_elements();
            }
        } else if (pressed & J_START) {
            reset_counters();
            draw_life();
            draw_elements();
        }

        vsync();
    }
}
