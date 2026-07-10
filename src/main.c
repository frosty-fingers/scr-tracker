#include <gbdk/platform.h>
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdio.h>
#include <stdint.h>
#include <gbdk/console.h>

#include "elements.h"

// NOTE: this project has no save data, so save.h (still present in
// src/) is intentionally not included here. Sound IS used (see
// play_tone() below) but that's just direct PSG register writes, not
// the music.h/hUGEDriver system - music.h is still unused.

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

// Elements are laid out as one row each, AIR/EARTH/FIRE/WATER top to
// bottom - a threshold's count is shown as that many repeated element
// symbols (up to MAX_SYMBOLS), not a number, so it reads at a glance.
static const uint8_t element_row[4]  = { 7u, 8u, 9u, 10u };
static const uint8_t icon_tile[4]    = { TILE_AIR, TILE_EARTH, TILE_FIRE, TILE_WATER };
// Labels are pre-padded to a fixed width so columns line up without
// relying on printf width specifiers (see docs/GOTCHAS.md - GBDK's
// printf doesn't reliably honor those).
static const char *element_label[4]  = { "AIR   ", "EARTH ", "FIRE  ", "WATER " };

#define COL_CURSOR    0u
#define COL_LABEL     1u
#define COL_SYMBOLS   7u
#define COL_OVERFLOW  15u
#define MAX_SYMBOLS   8u

#define ROW_TITLE1    0u
#define ROW_TITLE2    1u
#define ROW_LIFE_CUR  3u
#define ROW_LIFE      3u
#define ROW_DEATHDOOR 5u
#define ROW_HINT1     13u
#define ROW_HINT2     14u

// How long (in frames, ~60/sec) A/B must be held before auto-repeat
// kicks in, and how many frames between each repeated step after that.
#define REPEAT_DELAY     18u
#define REPEAT_INTERVAL  5u

// Placeholder SFX frequencies (PSG channel 1 period values, not Hz -
// see play_tone()). Roughly A5 for +1, A4 for -1, so they're clearly
// distinguishable by ear.
#define SND_FREQ_INC  1899u
#define SND_FREQ_DEC  1750u

static uint8_t counters[COUNTER_COUNT];
static uint8_t active = COUNTER_LIFE;

// Loads the element icon tiles. Must be called AFTER the font/console
// system has been "primed" (see prime_console() and main()) - see
// docs/GOTCHAS.md for why.
static void init_graphics(void) {
    set_bkg_data(TILE_FIRST_ELEMENT, 4u, element_tiles);
}

// The font/console system clears the background tile map the first
// time printf()/gotoxy() is used (see docs/GOTCHAS.md). Anything placed
// on the background before that first use gets wiped out. This forces
// that one-time setup to happen with a throwaway print, before any
// custom tiles are placed, so they survive.
static void prime_console(void) {
    gotoxy(0u, 0u);
    printf(" ");
}

// Turns the PSG on and routes channel 1 to both speakers at full
// volume. Called once at startup.
static void init_sound(void) {
    NR52_REG = 0x80u;
    NR50_REG = 0x77u;
    NR51_REG = 0x11u;
}

// Plays a short channel-1 "blip" that fades out quickly - a generic
// placeholder menu ding for +/- feedback. freq_reg is an 11-bit PSG
// period value (not Hz): higher value = higher pitch.
static void play_tone(uint16_t freq_reg) {
    NR10_REG = 0x00u;                              // no pitch sweep
    NR11_REG = 0x80u;                               // 50% duty
    NR12_REG = 0xF2u;                               // vol 15, fast decay
    NR13_REG = freq_reg & 0xFFu;
    NR14_REG = 0x80u | ((freq_reg >> 8u) & 0x07u);   // trigger, no length limit
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

// Draws everything that never changes: title, LIFE label, element
// labels, hints. Element labels are printed once here since (unlike
// their symbol counts) they never change.
static void draw_static_ui(void) {
    uint8_t i;

    gotoxy(6u, ROW_TITLE1);
    printf("SORCERY");
    gotoxy(4u, ROW_TITLE2);
    printf("LIFE TRACKER");

    gotoxy(3u, ROW_LIFE);
    printf("LIFE");

    for (i = 0u; i < 4u; i++) {
        gotoxy(COL_LABEL, element_row[i]);
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

// Redraws one element's row: its symbol count (as repeated icon tiles,
// capped at MAX_SYMBOLS with the exact total shown alongside if it goes
// over), and its cursor marker.
static void draw_element(uint8_t index) {
    uint8_t row_tiles[MAX_SYMBOLS];
    uint8_t count = counters[COUNTER_AIR + index];
    uint8_t shown = (count > MAX_SYMBOLS) ? MAX_SYMBOLS : count;
    uint8_t j;

    for (j = 0u; j < MAX_SYMBOLS; j++) {
        row_tiles[j] = (j < shown) ? icon_tile[index] : 0u;
    }
    set_bkg_tiles(COL_SYMBOLS, element_row[index], MAX_SYMBOLS, 1u, row_tiles);

    gotoxy(COL_OVERFLOW, element_row[index]);
    if (count > MAX_SYMBOLS) {
        printf("%u ", (unsigned int)count);
    } else {
        printf("   ");
    }

    gotoxy(COL_CURSOR, element_row[index]);
    printf((active == COUNTER_AIR + index) ? ">" : " ");
}

static void draw_elements(void) {
    uint8_t i;
    for (i = 0u; i < 4u; i++) {
        draw_element(i);
    }
}

// Returns 1 if the active counter is currently allowed to change, 0 if
// it's locked. LIFE locks at Death's Door (0) until START resets it -
// elements are never locked.
static uint8_t counter_is_locked(void) {
    return (active == COUNTER_LIFE) && (counters[COUNTER_LIFE] == LIFE_MIN);
}

// Applies +1/-1 to the active counter. Returns 1 if the value actually
// changed (so callers can skip redrawing/playing a sound when locked or
// already at a limit), 0 otherwise.
static uint8_t apply_delta(int8_t delta) {
    uint8_t value;
    uint8_t min;
    uint8_t max;

    if (counter_is_locked()) {
        return 0u;
    }

    value = counters[active];
    min = (active == COUNTER_LIFE) ? LIFE_MIN : ELEM_MIN;
    max = (active == COUNTER_LIFE) ? LIFE_MAX : ELEM_MAX;

    if (delta > 0) {
        if (value >= max) {
            return 0u;
        }
        value++;
    } else {
        if (value <= min) {
            return 0u;
        }
        value--;
    }

    counters[active] = value;
    return 1u;
}

static void redraw_active(void) {
    if (active == COUNTER_LIFE) {
        draw_life();
    } else {
        draw_element(active - COUNTER_AIR);
    }
}

void main(void) {
    uint8_t prev_keys = 0u;
    uint8_t keys, pressed;
    uint8_t a_hold = 0u, b_hold = 0u;

    if (_cpu == CGB_TYPE) {
        set_default_palette();
    }

    init_sound();
    prime_console();
    init_graphics();
    reset_counters();
    draw_static_ui();
    draw_life();
    draw_elements();

    SHOW_BKG;
    DISPLAY_ON;

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
                if (apply_delta(1)) {
                    redraw_active();
                    play_tone(SND_FREQ_INC);
                }
                a_hold = 0u;
            } else {
                a_hold++;
                if (a_hold >= REPEAT_DELAY &&
                    ((a_hold - REPEAT_DELAY) % REPEAT_INTERVAL) == 0u) {
                    if (apply_delta(1)) {
                        redraw_active();
                        play_tone(SND_FREQ_INC);
                    }
                }
            }
        } else {
            a_hold = 0u;
        }

        // B: -1 on press, then auto-repeat while held.
        if (keys & J_B) {
            if (pressed & J_B) {
                if (apply_delta(-1)) {
                    redraw_active();
                    play_tone(SND_FREQ_DEC);
                }
                b_hold = 0u;
            } else {
                b_hold++;
                if (b_hold >= REPEAT_DELAY &&
                    ((b_hold - REPEAT_DELAY) % REPEAT_INTERVAL) == 0u) {
                    if (apply_delta(-1)) {
                        redraw_active();
                        play_tone(SND_FREQ_DEC);
                    }
                }
            }
        } else {
            b_hold = 0u;
        }

        vsync();
    }
}
