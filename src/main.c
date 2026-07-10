#include <gbdk/platform.h>
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdio.h>
#include <stdint.h>
#include <gbdk/console.h>

#include "elements.h"

// NOTE: this project has no save data, so save.h (still present in
// src/) is intentionally not included here. Sound uses direct PSG
// register writes (play_tone() below), not the music.h/hUGEDriver
// system - music.h is still unused.

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

#define MAX_SYMBOLS   8u

// counters/active are indexed by player (0 or 1). In 1-player mode
// only index 0 is ever used. Palette slot order matches element order:
// 0 = AIR, 1 = EARTH, 2 = FIRE, 3 = WATER (see element_palettes in
// elements.h) - icon_tile[] below follows the same order.
static const uint8_t icon_tile[4]          = { TILE_AIR, TILE_EARTH, TILE_FIRE, TILE_WATER };
static const char *element_label_word[4]   = { "AIR   ", "EARTH ", "FIRE  ", "WATER " };
static const char *element_label_letter[4] = { "A", "E", "F", "W" };

static uint8_t counters[2][COUNTER_COUNT];
static uint8_t active[2];
static uint8_t current_player = 0u;
static uint8_t num_players = 1u;

#define STATE_TITLE  0u
#define STATE_PLAY   1u
static uint8_t game_state = STATE_TITLE;
static uint8_t title_selection = 0u;  // 0 = 1 player, 1 = 2 player

// How long (in frames, ~60/sec) A/B must be held before auto-repeat
// kicks in, and how many frames between each repeated step after that.
#define REPEAT_DELAY     18u
#define REPEAT_INTERVAL  5u

// Placeholder SFX frequencies (PSG channel 1 period values, not Hz).
#define SND_FREQ_INC  1899u
#define SND_FREQ_DEC  1750u

// ===================== sound ============================================

static void init_sound(void) {
    NR52_REG = 0x80u;
    NR50_REG = 0x77u;
    NR51_REG = 0x11u;
}

// Plays a short channel-1 "blip" that fades out quickly - a generic
// placeholder menu ding for +/- feedback.
static void play_tone(uint16_t freq_reg) {
    NR10_REG = 0x00u;
    NR11_REG = 0x80u;
    NR12_REG = 0xF2u;
    NR13_REG = freq_reg & 0xFFu;
    NR14_REG = 0x80u | ((freq_reg >> 8u) & 0x07u);
}

// ===================== console / graphics setup ========================

// The font/console system clears the background tile map the first
// time printf()/gotoxy() is used (see docs/GOTCHAS.md). This forces
// that one-time setup to happen with a throwaway print, before any
// custom tiles are placed, so they survive.
static void prime_console(void) {
    gotoxy(0u, 0u);
    printf(" ");
}

// Sets a solid color (palette slot 0-3, matching element order) across
// a horizontal run of background tiles. CGB only - no-op on DMG.
static void paint_row(uint8_t col, uint8_t row, uint8_t w, uint8_t pal) {
    uint8_t attr[MAX_SYMBOLS];
    uint8_t i;

    if (_cpu != CGB_TYPE) {
        return;
    }

    for (i = 0u; i < w; i++) {
        attr[i] = pal;
    }
    VBK_REG = VBK_ATTRIBUTES;
    set_bkg_tiles(col, row, w, 1u, attr);
    VBK_REG = VBK_TILES;
}

// ===================== shared counter logic =============================

static uint8_t counter_is_locked(uint8_t p) {
    return (active[p] == COUNTER_LIFE) && (counters[p][COUNTER_LIFE] == LIFE_MIN);
}

// Applies +1/-1 to player p's active counter. Returns 1 if the value
// actually changed, 0 if locked or already at a limit.
static uint8_t apply_delta(uint8_t p, int8_t delta) {
    uint8_t value;
    uint8_t min;
    uint8_t max;

    if (counter_is_locked(p)) {
        return 0u;
    }

    value = counters[p][active[p]];
    min = (active[p] == COUNTER_LIFE) ? LIFE_MIN : ELEM_MIN;
    max = (active[p] == COUNTER_LIFE) ? LIFE_MAX : ELEM_MAX;

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

    counters[p][active[p]] = value;
    return 1u;
}

static void reset_player(uint8_t p) {
    counters[p][COUNTER_LIFE] = LIFE_START;
    counters[p][COUNTER_AIR] = 0u;
    counters[p][COUNTER_EARTH] = 0u;
    counters[p][COUNTER_FIRE] = 0u;
    counters[p][COUNTER_WATER] = 0u;
    active[p] = COUNTER_LIFE;
}

// Prints a counter value left-aligned followed by a blanking space, so
// a value that shrinks by a digit doesn't leave a stale digit behind
// (see docs/GOTCHAS.md - GBDK's printf doesn't reliably honor width
// specifiers like "%2u").
static void print_value(uint8_t value) {
    printf("%u ", (unsigned int)value);
}

// ===================== title screen ======================================

static void title_draw(void) {
    gotoxy(6u, 2u);
    printf("SORCERY");
    gotoxy(4u, 3u);
    printf("LIFE TRACKER");

    gotoxy(4u, 7u);
    printf((title_selection == 0u) ? ">1 PLAYER" : " 1 PLAYER");
    gotoxy(4u, 9u);
    printf((title_selection == 1u) ? ">2 PLAYER" : " 2 PLAYER");

    gotoxy(1u, 15u);
    printf("<>SELECT A:START");
}

static void title_start_game(void) {
    num_players = (title_selection == 0u) ? 1u : 2u;
    current_player = 0u;
    reset_player(0u);
    reset_player(1u);
    game_state = STATE_PLAY;
    cls();
}

// ===================== 1-player UI (unchanged layout) ====================

#define P1_COL_CURSOR     0u
#define P1_COL_LABEL      1u
#define P1_COL_SYMBOLS    7u
#define P1_COL_OVERFLOW   15u
#define P1_ROW_TITLE1     0u
#define P1_ROW_TITLE2     1u
#define P1_ROW_LIFE       3u
#define P1_ROW_DEATHDOOR  5u
#define P1_ROW_HINT1      13u
#define P1_ROW_HINT2      14u

static const uint8_t p1_element_row[4] = { 7u, 8u, 9u, 10u };

static void solo_draw_static_ui(void) {
    uint8_t i;

    gotoxy(6u, P1_ROW_TITLE1);
    printf("SORCERY");
    gotoxy(4u, P1_ROW_TITLE2);
    printf("LIFE TRACKER");

    gotoxy(3u, P1_ROW_LIFE);
    printf("LIFE");

    for (i = 0u; i < 4u; i++) {
        gotoxy(P1_COL_LABEL, p1_element_row[i]);
        printf(element_label_word[i]);
        paint_row(P1_COL_SYMBOLS, p1_element_row[i], MAX_SYMBOLS, i);
    }

    gotoxy(0u, P1_ROW_HINT1);
    printf("<>SEL A:+ B:-");
    gotoxy(0u, P1_ROW_HINT2);
    printf("START:RESET");
}

static void solo_draw_life(void) {
    gotoxy(0u, P1_ROW_LIFE);
    printf((active[0] == COUNTER_LIFE) ? ">" : " ");

    gotoxy(9u, P1_ROW_LIFE);
    print_value(counters[0][COUNTER_LIFE]);

    gotoxy(2u, P1_ROW_DEATHDOOR);
    printf((counters[0][COUNTER_LIFE] == LIFE_MIN) ? "  DEATHS DOOR   " : "                ");
}

static void solo_draw_element(uint8_t idx) {
    uint8_t row_tiles[MAX_SYMBOLS];
    uint8_t count = counters[0][COUNTER_AIR + idx];
    uint8_t shown = (count > MAX_SYMBOLS) ? MAX_SYMBOLS : count;
    uint8_t j;

    for (j = 0u; j < MAX_SYMBOLS; j++) {
        row_tiles[j] = (j < shown) ? icon_tile[idx] : 0u;
    }
    set_bkg_tiles(P1_COL_SYMBOLS, p1_element_row[idx], MAX_SYMBOLS, 1u, row_tiles);

    gotoxy(P1_COL_OVERFLOW, p1_element_row[idx]);
    if (count > MAX_SYMBOLS) {
        printf("%u ", (unsigned int)count);
    } else {
        printf("   ");
    }

    gotoxy(P1_COL_CURSOR, p1_element_row[idx]);
    printf((active[0] == COUNTER_AIR + idx) ? ">" : " ");
}

static void solo_draw_elements(void) {
    uint8_t i;
    for (i = 0u; i < 4u; i++) {
        solo_draw_element(i);
    }
}

static void solo_redraw_active(void) {
    if (active[0] == COUNTER_LIFE) {
        solo_draw_life();
    } else {
        solo_draw_element(active[0] - COUNTER_AIR);
    }
}

// ===================== 2-player UI (condensed, mirrored) =================
//
// Screen is split into a left half (cols 0-9, player 0) and a right
// half (cols 10-19, player 1). Player 1's side is a mirror image of
// player 0's: cursor/label sit on the outer edge (col 19 instead of
// col 0) and symbols grow leftward from the label instead of
// rightward, so the two halves visually face each other.

#define TWO_ROW_HEADER  0u
#define TWO_ROW_LIFE    2u
#define TWO_ROW_DOOR    3u
#define TWO_ROW_ELEM0   5u   // elements at rows 5,6,7,8 (AIR/EARTH/FIRE/WATER)
#define TWO_ROW_HINT1   11u
#define TWO_ROW_HINT2   12u

static const uint8_t two_life_cursor_col[2] = { 0u, 19u };
static const uint8_t two_life_label_col[2]  = { 1u, 15u };
static const uint8_t two_life_value_col[2]  = { 6u, 11u };
static const uint8_t two_door_col[2]        = { 1u, 15u };
static const uint8_t two_header_col[2]      = { 0u, 16u };
static const uint8_t two_elem_cursor_col[2] = { 0u, 19u };
static const uint8_t two_elem_label_col[2]  = { 1u, 18u };
static const uint8_t two_elem_attr_col[2]   = { 2u, 10u };  // leftmost col of the 8-cell symbol block

static void two_draw_static_ui(void) {
    uint8_t i;

    gotoxy(1u, TWO_ROW_HINT1);
    printf("SEL:SWAP <>A+B-");
    gotoxy(1u, TWO_ROW_HINT2);
    printf("START:RESET(cur)");

    for (i = 0u; i < 4u; i++) {
        paint_row(two_elem_attr_col[0], TWO_ROW_ELEM0 + i, MAX_SYMBOLS, i);
        paint_row(two_elem_attr_col[1], TWO_ROW_ELEM0 + i, MAX_SYMBOLS, i);
    }
}

static void two_draw_header(void) {
    gotoxy(two_header_col[0], TWO_ROW_HEADER);
    printf((current_player == 0u) ? "[P1]" : " P1 ");
    gotoxy(two_header_col[1], TWO_ROW_HEADER);
    printf((current_player == 1u) ? "[P2]" : " P2 ");
}

static void two_draw_life(uint8_t p) {
    gotoxy(two_life_cursor_col[p], TWO_ROW_LIFE);
    printf((active[p] == COUNTER_LIFE) ? ">" : " ");

    gotoxy(two_life_label_col[p], TWO_ROW_LIFE);
    printf("LIFE");

    gotoxy(two_life_value_col[p], TWO_ROW_LIFE);
    print_value(counters[p][COUNTER_LIFE]);

    gotoxy(two_door_col[p], TWO_ROW_DOOR);
    printf((counters[p][COUNTER_LIFE] == LIFE_MIN) ? "DOOR" : "    ");
}

static void two_draw_element(uint8_t p, uint8_t idx) {
    uint8_t block[MAX_SYMBOLS];
    uint8_t count = counters[p][COUNTER_AIR + idx];
    uint8_t shown = (count > MAX_SYMBOLS) ? MAX_SYMBOLS : count;
    uint8_t j;
    uint8_t row = TWO_ROW_ELEM0 + idx;

    if (p == 0u) {
        for (j = 0u; j < MAX_SYMBOLS; j++) {
            block[j] = (j < shown) ? icon_tile[idx] : 0u;
        }
    } else {
        for (j = 0u; j < MAX_SYMBOLS; j++) {
            block[j] = 0u;
        }
        for (j = 0u; j < shown; j++) {
            block[(MAX_SYMBOLS - 1u) - j] = icon_tile[idx];
        }
    }
    set_bkg_tiles(two_elem_attr_col[p], row, MAX_SYMBOLS, 1u, block);

    gotoxy(two_elem_label_col[p], row);
    printf(element_label_letter[idx]);

    gotoxy(two_elem_cursor_col[p], row);
    printf((active[p] == COUNTER_AIR + idx) ? ">" : " ");
}

static void two_draw_elements(uint8_t p) {
    uint8_t i;
    for (i = 0u; i < 4u; i++) {
        two_draw_element(p, i);
    }
}

static void two_redraw_active(uint8_t p) {
    if (active[p] == COUNTER_LIFE) {
        two_draw_life(p);
    } else {
        two_draw_element(p, active[p] - COUNTER_AIR);
    }
}

// ===================== shared redraw dispatch =============================

static void redraw_active_current(void) {
    if (num_players == 1u) {
        solo_redraw_active();
    } else {
        two_redraw_active(current_player);
    }
}

static void redraw_life_and_elements_current(void) {
    if (num_players == 1u) {
        solo_draw_life();
        solo_draw_elements();
    } else {
        two_draw_life(current_player);
        two_draw_elements(current_player);
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
    set_bkg_data(TILE_FIRST_ELEMENT, 4u, element_tiles);
    if (_cpu == CGB_TYPE) {
        set_bkg_palette(0u, 4u, element_palettes);
    }

    title_draw();

    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        keys = joypad();
        pressed = keys & (uint8_t)(~prev_keys);
        prev_keys = keys;

        if (game_state == STATE_TITLE) {
            if (pressed & (J_UP | J_DOWN)) {
                title_selection = 1u - title_selection;
                title_draw();
            }
            if (pressed & (J_A | J_START)) {
                title_start_game();
                if (num_players == 1u) {
                    solo_draw_static_ui();
                    solo_draw_life();
                    solo_draw_elements();
                } else {
                    two_draw_static_ui();
                    two_draw_header();
                    two_draw_life(0u);
                    two_draw_life(1u);
                    two_draw_elements(0u);
                    two_draw_elements(1u);
                }
            }
        } else {
            if (num_players == 2u && (pressed & J_SELECT)) {
                current_player = 1u - current_player;
                two_draw_header();
            }

            if (pressed & J_LEFT) {
                active[current_player] = (active[current_player] == COUNTER_LIFE)
                    ? (COUNTER_COUNT - 1u) : (active[current_player] - 1u);
                redraw_life_and_elements_current();
            } else if (pressed & J_RIGHT) {
                active[current_player] = (active[current_player] == COUNTER_COUNT - 1u)
                    ? COUNTER_LIFE : (active[current_player] + 1u);
                redraw_life_and_elements_current();
            } else if (pressed & J_START) {
                reset_player(current_player);
                redraw_life_and_elements_current();
            }

            // A: +1 on press, then auto-repeat while held.
            if (keys & J_A) {
                if (pressed & J_A) {
                    if (apply_delta(current_player, 1)) {
                        redraw_active_current();
                        play_tone(SND_FREQ_INC);
                    }
                    a_hold = 0u;
                } else {
                    a_hold++;
                    if (a_hold >= REPEAT_DELAY &&
                        ((a_hold - REPEAT_DELAY) % REPEAT_INTERVAL) == 0u) {
                        if (apply_delta(current_player, 1)) {
                            redraw_active_current();
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
                    if (apply_delta(current_player, -1)) {
                        redraw_active_current();
                        play_tone(SND_FREQ_DEC);
                    }
                    b_hold = 0u;
                } else {
                    b_hold++;
                    if (b_hold >= REPEAT_DELAY &&
                        ((b_hold - REPEAT_DELAY) % REPEAT_INTERVAL) == 0u) {
                        if (apply_delta(current_player, -1)) {
                            redraw_active_current();
                            play_tone(SND_FREQ_DEC);
                        }
                    }
                }
            } else {
                b_hold = 0u;
            }
        }

        vsync();
    }
}
