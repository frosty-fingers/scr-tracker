#include <gbdk/platform.h>
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdio.h>
#include <stdint.h>
#include <gbdk/console.h>
#include <rand.h>

#include "elements.h"
#include "avatars.h"
#include "glossary.h"
#include "printer.h"

// NOTE: this project has no save data, so save.h (still present in
// src/) is intentionally not included here. Sound uses direct PSG
// register writes (play_tone() below), not the music.h/hUGEDriver
// system - music.h is still unused. Background music is intentionally
// out of scope right now (see docs/STATUS.md).

// BUILD_NUMBER is passed in by the Makefile as a bare token
// (-DBUILD_NUMBER=49, no quotes) - in CI it's the GitHub Actions run
// number, which auto-increments on every build with no extra state to
// track (see .github/workflows/build-rom.yml and tools/release.sh,
// which use the same number for the release .gbc's filename, so the
// title screen and the filename always match). BUILD_NUMBER_STR turns
// that bare token into an actual string literal at compile time via
// the standard two-macro stringize trick (the extra indirection through
// STRINGIFY_HELPER matters - a single-level "#x" would stringize the
// literal text "BUILD_NUMBER" instead of its expanded value). Passing
// the raw value with no quotes at the Makefile level, rather than
// embedding quote characters in the Makefile itself, sidesteps a real
// Makefile-quoting issue hit in CI (see docs/GOTCHAS.md).
#ifndef BUILD_NUMBER
#define BUILD_NUMBER dev
#endif
#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)
#define BUILD_NUMBER_STR STRINGIFY(BUILD_NUMBER)

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
// only index 0 is ever used. Palette slot order matches element order
// (see theme_palettes in elements.h): element index 0=AIR,1=EARTH,2=FIRE,
// 3=WATER maps to CGB palette slot index+1 (slot 0 is reserved for
// default text - see the big comment in elements.h about that bug).
static const uint8_t icon_tile[4]          = { TILE_AIR, TILE_EARTH, TILE_FIRE, TILE_WATER };
static const char *element_label_word[4]   = { "AIR   ", "EARTH ", "FIRE  ", "WATER " };
static const char *element_label_letter[4] = { "A", "E", "F", "W" };

static uint8_t counters[2][COUNTER_COUNT];
static uint8_t active[2];
static uint8_t current_player = 0u;
static uint8_t num_players = 1u;
static uint8_t avatar_choice[2] = { 0u, 0u };

#define STATE_TITLE           0u
#define STATE_AVATAR_P1       1u
#define STATE_AVATAR_P2       2u
#define STATE_PLAY            3u
#define STATE_SETTINGS        4u
#define STATE_DICE            5u
#define STATE_GLOSSARY_LIST   6u
#define STATE_GLOSSARY_DETAIL 7u
static uint8_t game_state = STATE_TITLE;
static uint8_t title_selection = 0u;  // 0=1 player,1=2 player,2=settings,3=glossary
static uint8_t theme_choice = 0u;

// Glossary - opened either from the title screen or, mid-match, with
// SELECT+DOWN (DOWN was the last genuinely free input during play -
// UP is already the dice roller's shortcut). glossary_return_state
// remembers which of those two places B should go back to once the
// person leaves the glossary entirely (from the list screen -
// backing out of a single entry's detail view always just returns to
// the list, regardless of how the glossary was opened).
static uint8_t glossary_return_state = STATE_TITLE;
static uint8_t glossary_current_page = PAGE_KEYWORDS;
static uint8_t glossary_cursor = 0u;  // index WITHIN the current page, not into glossary_terms[] directly
static uint8_t glossary_scroll = 0u;

// How many glossary_terms[] entries belong to the given page.
static uint8_t glossary_page_count(uint8_t page) {
    uint8_t count = 0u;
    uint8_t i;
    for (i = 0u; i < GLOSSARY_COUNT; i++) {
        if (glossary_page[i] == page) {
            count++;
        }
    }
    return count;
}

// Resolves a page-relative position (the "nth entry on this page", 0-
// based - what glossary_cursor actually holds) to its real index into
// glossary_terms[]/glossary_def_lines[]. Entries for a page don't need
// to be contiguous in those arrays, so this always does a fresh scan
// rather than assuming any particular layout - GLOSSARY_COUNT is small
// enough (called only on button presses, not per-frame) that this
// isn't a performance concern.
static uint8_t glossary_page_nth_index(uint8_t page, uint8_t n) {
    uint8_t i;
    uint8_t seen = 0u;
    for (i = 0u; i < GLOSSARY_COUNT; i++) {
        if (glossary_page[i] == page) {
            if (seen == n) {
                return i;
            }
            seen++;
        }
    }
    return 0u;  // shouldn't happen if n < glossary_page_count(page)
}

// Dice/coin roller - opened during a match with SELECT+UP (see the
// STATE_PLAY input handling), since every single button is already
// spoken for during gameplay. dice_result of 0 means "not rolled yet".
#define DICE_D20         0u
#define DICE_D6          1u
#define DICE_COIN        2u
#define DICE_HARBINGER   3u
#define DICE_TYPE_COUNT  4u
static const uint8_t dice_sides[DICE_TYPE_COUNT] = { 20u, 6u, 2u, 0u };  // 0 unused for HARBINGER
static const char *dice_type_label[DICE_TYPE_COUNT] = { "D20", "D6", "COIN", "HARBINGER" };
static uint8_t dice_type = 0u;
static uint8_t dice_result = 0u;

// Harbinger's 4x5 grid - grid_highlighted[i] is 1 if square i (row-
// major, row = i/GRID_COLS, col = i%GRID_COLS) is one of the 3 chosen
// squares this roll, 0 otherwise. All start unhighlighted (hollow)
// until the first roll.
#define GRID_COLS  5u
#define GRID_ROWS  4u
#define GRID_SQUARES  (GRID_COLS * GRID_ROWS)
#define GRID_HIGHLIGHT_COUNT  3u
static uint8_t grid_highlighted[GRID_SQUARES];

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

static void play_tone(uint16_t freq_reg) {
    NR10_REG = 0x00u;
    NR11_REG = 0x80u;
    NR12_REG = 0xF2u;
    NR13_REG = freq_reg & 0xFFu;
    NR14_REG = 0x80u | ((freq_reg >> 8u) & 0x07u);
}

// Loads the currently chosen theme's palettes (see theme_palettes in
// elements.h). CGB only - no-op on DMG. Safe to call any time,
// including live while the settings screen is open, since palette RAM
// can be updated whenever - no need to be off-screen/off-display for
// this one (unlike the tile-placement ordering rule in
// prime_console()'s comment).
static void apply_theme(void) {
    if (_cpu != CGB_TYPE) {
        return;
    }
    set_bkg_palette(0u, 6u, theme_palettes[theme_choice]);
}

// Rolls a die with the given number of sides (2 for a coin flip),
// returning 1..sides. Reseeds from DIV_REG (the free-running timer
// register) on every roll rather than once at startup - DIV changes
// constantly and real human button-press timing is unpredictable at
// that resolution, so this is a simple, reliable way to get a fresh,
// non-repeating result each time without needing a higher-quality RNG
// than this hardware actually has.
static uint8_t dice_roll(uint8_t sides) {
    initrand(DIV_REG);
    return ((uint8_t)rand() % sides) + 1u;
}

// Clears every square back to unhighlighted (hollow) - called both at
// the start of a fresh roll and whenever the dice/coin type changes
// away from or back to Harbinger, so switching types and coming back
// never shows a stale previous roll.
static void grid_reset(void) {
    uint8_t i;
    for (i = 0u; i < GRID_SQUARES; i++) {
        grid_highlighted[i] = 0u;
    }
}

// Picks GRID_HIGHLIGHT_COUNT (3) unique squares out of GRID_SQUARES
// (20) at random, no duplicates. Seeds DIV_REG *once* per roll, not
// once per pick - reseeding inside the loop would sample DIV_REG
// several times within the same button press, close enough together
// that it could return the same (or a very similar) value each time,
// defeating the whole point of using it as an unpredictable seed (see
// dice_roll() above, which only needs one value so this doesn't apply
// there). Rejection sampling from a single seeded sequence instead -
// with only 20 possible values and 3 needed, this settles quickly.
static void roll_grid(void) {
    uint8_t count = 0u;
    uint8_t idx;

    grid_reset();

    initrand(DIV_REG);
    while (count < GRID_HIGHLIGHT_COUNT) {
        idx = (uint8_t)rand() % GRID_SQUARES;
        if (!grid_highlighted[idx]) {
            grid_highlighted[idx] = 1u;
            count++;
        }
    }
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

// Sets a color (CGB palette slot, 1-4 for elements - see elements.h)
// across a horizontal run of background tiles. CGB only - no-op on DMG.
static void paint_row(uint8_t col, uint8_t row, uint8_t w, uint8_t pal) {
    // Sized to the screen's full width (20), not MAX_SYMBOLS (8) - this
    // is called with widths up to 16 (the "DEATHS DOOR" message), and
    // an 8-byte buffer there was a real stack buffer overflow corrupting
    // whatever else was on the stack. That's what caused 1-player mode
    // to render garbage/blank - see docs/GOTCHAS.md.
    uint8_t attr[20];
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

// Named CGB palette slots (see theme_palettes in elements.h) - the
// element slots (1-4) already come from icon_tile[]'s own index+1
// wherever a whole row of symbols gets colored, but FIRE_PAL and
// ACCENT_PAL get used directly by name in a few specific spots (the
// Death's Door warning, and every selector indicator), so they're
// named here for clarity rather than left as bare numbers there.
#define TEXT_PAL    0u
#define FIRE_PAL    3u
#define ACCENT_PAL  5u

// Colors a single cell as the theme's accent color - used for every
// selector indicator (">"/"<" cursors, "[" "]" brackets) so they pop
// as a highlight instead of blending into the regular text color.
static void paint_selector(uint8_t col, uint8_t row) {
    paint_row(col, row, 1u, ACCENT_PAL);
}

// cls() only clears the background tile ID plane, not the CGB
// attribute (palette) plane - so leftover paint_row() colors from a
// previous screen/mode stick around indefinitely on whatever cells
// they were last applied to, bleeding into new text that happens to
// land on the same row/column later (see docs/GOTCHAS.md). This resets
// the whole visible screen's attributes back to palette slot 0 (the
// default text palette). CGB only - no-op on DMG.
static void clear_attributes(void) {
    uint8_t attr[20];
    uint8_t y;
    uint8_t i;

    if (_cpu != CGB_TYPE) {
        return;
    }

    for (i = 0u; i < 20u; i++) {
        attr[i] = 0u;
    }
    VBK_REG = VBK_ATTRIBUTES;
    for (y = 0u; y < 18u; y++) {
        set_bkg_tiles(0u, y, 20u, 1u, attr);
    }
    VBK_REG = VBK_TILES;
}

// Full screen reset before drawing a new screen: clears both tile IDs
// (cls()) and leftover color attributes (clear_attributes()). Use this
// instead of a bare cls() anywhere the game transitions between
// screens/modes.
static void full_clear(void) {
    cls();
    clear_attributes();
}

static uint8_t counter_is_locked(uint8_t p) {
    return (active[p] == COUNTER_LIFE) && (counters[p][COUNTER_LIFE] == LIFE_MIN);
}

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

static void print_value(uint8_t value) {
    printf("%u ", (unsigned int)value);
}

// Prints at most maxlen characters of s, stopping early at the null
// terminator either way. Used anywhere a string comes from a data file
// meant to be hand-edited (avatars.h, glossary.h) - if someone types
// an entry longer than the space available for it, this clips it
// instead of letting it overflow into neighboring text or off the
// edge of the screen. Doesn't need any of the printf/%s varargs
// casting caveats from docs/GOTCHAS.md since putchar() takes a single
// fixed argument, not a variadic one.
static void print_clipped(const char *s, uint8_t maxlen) {
    uint8_t i = 0u;
    while (s[i] != '\0' && i < maxlen) {
        putchar(s[i]);
        i++;
    }
}

// ===================== title & avatar select screens ====================

static void title_draw(void) {
    gotoxy(6u, 2u);
    printf("SORCERY");
    gotoxy(4u, 3u);
    printf("LIFE TRACKER");

    gotoxy(4u, 6u);
    printf((title_selection == 0u) ? ">1 PLAYER" : " 1 PLAYER");
    paint_selector(4u, 6u);
    gotoxy(4u, 8u);
    printf((title_selection == 1u) ? ">2 PLAYER" : " 2 PLAYER");
    paint_selector(4u, 8u);
    gotoxy(4u, 10u);
    printf((title_selection == 2u) ? ">SETTINGS" : " SETTINGS");
    paint_selector(4u, 10u);
    gotoxy(4u, 12u);
    printf((title_selection == 3u) ? ">GLOSSARY" : " GLOSSARY");
    paint_selector(4u, 12u);

    gotoxy(1u, 15u);
    printf("SELECT A:START");

    // Build number - see the BUILD_NUMBER comment near the top of this
    // file. Cleared first since it varies in length ("dev" vs. a CI
    // run number that grows over time) - same pattern as every other
    // variable-length field in this project (see docs/GOTCHAS.md).
    gotoxy(1u, 17u);
    printf("          ");  // 10 spaces
    gotoxy(1u, 17u);
    printf("V");
    printf(BUILD_NUMBER_STR);
}

#define SETTINGS_NAME_COL         1u
#define SETTINGS_NAME_MAXLEN      17u
#define SETTINGS_ARROW_LEFT_COL   0u
#define SETTINGS_ARROW_RIGHT_COL  19u

static void settings_draw(void) {
    gotoxy(5u, 6u);
    printf("THEME");

    gotoxy(SETTINGS_ARROW_LEFT_COL, 9u);
    printf("<");
    paint_selector(SETTINGS_ARROW_LEFT_COL, 9u);
    gotoxy(SETTINGS_ARROW_RIGHT_COL, 9u);
    printf(">");
    paint_selector(SETTINGS_ARROW_RIGHT_COL, 9u);

    // Clear the full name field before printing - theme names vary in
    // length (see docs/GOTCHAS.md's digit/name-clearing pattern) - and
    // clip to the field width in case a future theme name is too long
    // to fit (see print_clipped()'s comment).
    gotoxy(SETTINGS_NAME_COL, 9u);
    printf("                 ");  // 17 spaces
    gotoxy(SETTINGS_NAME_COL, 9u);
    print_clipped(theme_names[theme_choice], SETTINGS_NAME_MAXLEN);

    gotoxy(1u, 15u);
    printf("<>PICK B:BACK");
}

#define AVATAR_NAME_COL   1u
#define AVATAR_ARROW_LEFT_COL   0u
#define AVATAR_ARROW_RIGHT_COL  19u

static void avatar_draw(uint8_t p) {
    gotoxy(4u, 6u);
    printf((p == 0u) ? "CHOOSE P1" : "CHOOSE P2");

    gotoxy(AVATAR_ARROW_LEFT_COL, 9u);
    printf("<");
    paint_selector(AVATAR_ARROW_LEFT_COL, 9u);
    gotoxy(AVATAR_ARROW_RIGHT_COL, 9u);
    printf(">");
    paint_selector(AVATAR_ARROW_RIGHT_COL, 9u);

    // Clear the full name field before printing the new name - names
    // vary in length, so without this a shorter name would leave
    // stale characters from a longer previous one (same issue as the
    // digit-clearing fix - see docs/GOTCHAS.md) - and clip to the
    // field width in case an edited avatars.h entry is too long to
    // fit (see print_clipped()'s comment).
    gotoxy(AVATAR_NAME_COL, 9u);
    printf("                 ");  // AVATAR_NAME_MAXLEN (17) spaces
    gotoxy(AVATAR_NAME_COL, 9u);
    print_clipped(avatar_names[avatar_choice[p]], AVATAR_NAME_MAXLEN);

    gotoxy(1u, 15u);
    printf("<>PICK A:OK");
}

#define DICE_TYPE_COL         3u
#define DICE_TYPE_ARROW_LEFT  1u
#define DICE_TYPE_ARROW_RIGHT 19u
#define DICE_RESULT_COL       1u
#define DICE_ROW_TITLE        2u
#define DICE_ROW_TYPE         6u
#define DICE_ROW_RESULT       10u
#define DICE_ROW_HINT1        14u
#define DICE_ROW_HINT2        15u

#define GRID_COL0      5u
#define GRID_ROW0      8u
#define GRID_COL_STEP  2u
// Total columns the grid visually spans, accounting for the gaps
// between squares (GRID_COL_STEP) - NOT the same as GRID_COLS (the
// number of squares). Squares sit at GRID_COL0, GRID_COL0+2,
// GRID_COL0+4, GRID_COL0+6, GRID_COL0+8, so the span is 9 columns
// wide, not 5 - clearing only GRID_COLS columns would miss the
// rightmost squares entirely.
#define GRID_SPAN_COLS  ((GRID_COLS - 1u) * GRID_COL_STEP + 1u)

// Draws all 20 grid squares - hollow by default, filled (and accent-
// colored on CGB) for whichever 3 are in grid_highlighted[].
static void draw_grid(void) {
    uint8_t r, c, idx, tile;

    for (r = 0u; r < GRID_ROWS; r++) {
        for (c = 0u; c < GRID_COLS; c++) {
            idx = (r * GRID_COLS) + c;
            tile = grid_highlighted[idx] ? TILE_GRID_FILLED : TILE_GRID_EMPTY;
            set_bkg_tiles(GRID_COL0 + (c * GRID_COL_STEP), GRID_ROW0 + r, 1u, 1u, &tile);
            paint_row(GRID_COL0 + (c * GRID_COL_STEP), GRID_ROW0 + r, 1u,
                      grid_highlighted[idx] ? ACCENT_PAL : TEXT_PAL);
        }
    }
}

// Blanks both the normal text-result field and the grid area - called
// every time regardless of dice_type, so switching between a text
// result (D20/D6/COIN) and the Harbinger grid never leaves stale
// content from the other mode behind (same "always clear before
// drawing, don't assume the old state" pattern as everywhere else in
// this project - see docs/GOTCHAS.md).
// Blanks both the normal text-result field and the grid area - called
// every time regardless of dice_type, so switching between a text
// result (D20/D6/COIN) and the Harbinger grid never leaves stale
// content from the other mode behind (same "always clear before
// drawing, don't assume the old state" pattern as everywhere else in
// this project - see docs/GOTCHAS.md).
//
// This resets BOTH the tile IDs and the color attribute plane. Tile
// IDs alone isn't enough: a highlighted grid square's cell keeps its
// accent-color attribute even after the tile underneath it is blanked
// (paint_row() and set_bkg_tiles() touch separate VRAM planes - see
// the cls()-doesn't-clear-attributes gotcha). Without repainting the
// attribute plane too, switching away from Harbinger and printing a
// normal text result on the same row would render that text in
// whatever accent color was last left on those specific cells.
static void clear_dice_result_area(void) {
    uint8_t blank[GRID_SPAN_COLS];
    uint8_t r;
    uint8_t i;

    gotoxy(DICE_RESULT_COL, DICE_ROW_RESULT);
    printf("                  ");  // 18 spaces
    paint_row(DICE_RESULT_COL, DICE_ROW_RESULT, 18u, TEXT_PAL);

    for (i = 0u; i < GRID_SPAN_COLS; i++) {
        blank[i] = 0u;
    }
    for (r = 0u; r < GRID_ROWS; r++) {
        set_bkg_tiles(GRID_COL0, GRID_ROW0 + r, GRID_SPAN_COLS, 1u, blank);
        paint_row(GRID_COL0, GRID_ROW0 + r, GRID_SPAN_COLS, TEXT_PAL);
    }
}

static void dice_draw(void) {
    gotoxy(4u, DICE_ROW_TITLE);
    printf("DICE / COIN");

    gotoxy(DICE_TYPE_ARROW_LEFT, DICE_ROW_TYPE);
    printf("<");
    paint_selector(DICE_TYPE_ARROW_LEFT, DICE_ROW_TYPE);
    gotoxy(DICE_TYPE_ARROW_RIGHT, DICE_ROW_TYPE);
    printf(">");
    paint_selector(DICE_TYPE_ARROW_RIGHT, DICE_ROW_TYPE);

    // Clear the type-name field first - "D20"/"D6"/"COIN"/"HARBINGER"
    // differ in length (same reasoning as the avatar-name-clearing fix
    // above).
    gotoxy(DICE_TYPE_COL, DICE_ROW_TYPE);
    printf("                ");  // 16 spaces
    gotoxy(DICE_TYPE_COL, DICE_ROW_TYPE);
    printf(dice_type_label[dice_type]);

    clear_dice_result_area();

    if (dice_type == DICE_HARBINGER) {
        draw_grid();
    } else {
        gotoxy(DICE_RESULT_COL, DICE_ROW_RESULT);
        if (dice_result == 0u) {
            printf("PRESS A TO ROLL");
        } else if (dice_type == DICE_COIN) {
            printf((dice_result == 1u) ? "HEADS" : "TAILS");
        } else {
            printf("RESULT: %u", (unsigned int)dice_result);
        }
    }

    gotoxy(1u, DICE_ROW_HINT1);
    printf("<>TYPE A:ROLL");
    gotoxy(1u, DICE_ROW_HINT2);
    printf("B:BACK");
}

#define GLOSSARY_LIST_COL0      1u
#define GLOSSARY_LIST_ROW0      2u
#define GLOSSARY_VISIBLE_ROWS   13u
#define GLOSSARY_LIST_HINT_ROW1 16u
#define GLOSSARY_LIST_HINT_ROW2 17u

// Keeps the scroll window containing whatever's currently selected -
// works the same way whether the cursor moved one step or jumped
// straight from the last entry to the first (wraparound), since it
// just re-clamps the window around wherever the cursor ended up
// rather than incrementing/decrementing the scroll position directly.
static void glossary_clamp_scroll(void) {
    if (glossary_cursor < glossary_scroll) {
        glossary_scroll = glossary_cursor;
    } else if (glossary_cursor >= glossary_scroll + GLOSSARY_VISIBLE_ROWS) {
        glossary_scroll = glossary_cursor - GLOSSARY_VISIBLE_ROWS + 1u;
    }
}

#define GLOSSARY_PAGE_ARROW_LEFT   0u
#define GLOSSARY_PAGE_ARROW_RIGHT  19u
#define GLOSSARY_PAGE_NAME_COL     2u

static void glossary_list_draw(void) {
    uint8_t i;
    uint8_t filtered_idx;
    uint8_t actual_idx;
    uint8_t row;
    uint8_t page_count = glossary_page_count(glossary_current_page);

    gotoxy(GLOSSARY_PAGE_ARROW_LEFT, 0u);
    printf("<");
    paint_selector(GLOSSARY_PAGE_ARROW_LEFT, 0u);
    gotoxy(GLOSSARY_PAGE_ARROW_RIGHT, 0u);
    printf(">");
    paint_selector(GLOSSARY_PAGE_ARROW_RIGHT, 0u);

    // Clear the page name field first - "KEYWORDS"/"TOKENS" (or
    // whatever pages get added later) differ in length.
    gotoxy(GLOSSARY_PAGE_NAME_COL, 0u);
    printf("                ");  // GLOSSARY_PAGE_NAME_MAXLEN (16) spaces
    gotoxy(GLOSSARY_PAGE_NAME_COL, 0u);
    print_clipped(glossary_page_names[glossary_current_page], GLOSSARY_PAGE_NAME_MAXLEN);

    for (i = 0u; i < GLOSSARY_VISIBLE_ROWS; i++) {
        row = GLOSSARY_LIST_ROW0 + i;
        filtered_idx = i + glossary_scroll;

        gotoxy(GLOSSARY_LIST_COL0, row);
        printf("                 ");  // clear the row first (17 spaces) - see docs/GOTCHAS.md
        paint_selector(GLOSSARY_LIST_COL0, row);

        if (filtered_idx < page_count) {
            actual_idx = glossary_page_nth_index(glossary_current_page, filtered_idx);
            gotoxy(GLOSSARY_LIST_COL0, row);
            printf((filtered_idx == glossary_cursor) ? ">" : " ");
            // Leave 1 fewer character than the field's raw width for
            // the term itself, since the cursor already used one, and
            // clip in case an edited glossary.h entry is too long to
            // fit (see print_clipped()'s comment).
            print_clipped(glossary_terms[actual_idx], GLOSSARY_TERM_MAXLEN - 1u);
        }
    }

    gotoxy(1u, GLOSSARY_LIST_HINT_ROW1);
    printf("<>PG UP/DN A:VIEW");
    gotoxy(1u, GLOSSARY_LIST_HINT_ROW2);
    printf("B:BACK");
}

#define PRINT_STATUS_ROW      14u
#define PRINT_STATUS_MAXLEN   17u  // longest message ("NO PRINTER FOUND") is 16 chars

// Clears then prints a status line for the glossary print feature -
// same "clear the full field first" pattern as everywhere else
// variable-length text gets redrawn (see docs/GOTCHAS.md). Widths of
// "PRINTING..." / "PRINTED!" / "NO PRINTER FOUND" all differ.
static void print_status_line(const char *msg) {
    gotoxy(1u, PRINT_STATUS_ROW);
    printf("                 ");  // PRINT_STATUS_MAXLEN (17) spaces
    gotoxy(1u, PRINT_STATUS_ROW);
    print_clipped(msg, PRINT_STATUS_MAXLEN);
}

static void glossary_detail_draw(void) {
    uint8_t i;
    uint8_t actual_idx = glossary_page_nth_index(glossary_current_page, glossary_cursor);

    gotoxy(1u, 1u);
    printf("                 ");  // 17 spaces
    gotoxy(1u, 1u);
    print_clipped(glossary_terms[actual_idx], GLOSSARY_TERM_MAXLEN);

    for (i = 0u; i < GLOSSARY_MAX_LINES; i++) {
        gotoxy(1u, 4u + i);
        print_clipped(glossary_def_lines[actual_idx][i], GLOSSARY_LINE_MAXLEN);
    }

    gotoxy(1u, 16u);
    printf("A:PRINT B:BACK");
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
#define P1_ROW_HINT3      15u
#define P1_ROW_HINT4      16u

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
        paint_row(P1_COL_SYMBOLS, p1_element_row[i], MAX_SYMBOLS, i + 1u);
    }

    gotoxy(0u, P1_ROW_HINT1);
    printf("SEL A:+ B:-");
    gotoxy(0u, P1_ROW_HINT2);
    printf("START:RESET");
    gotoxy(0u, P1_ROW_HINT3);
    printf("SEL+UP:DICE");
    gotoxy(0u, P1_ROW_HINT4);
    printf("SEL+DN:GLOSS");
}

static void solo_draw_life(void) {
    gotoxy(0u, P1_ROW_LIFE);
    printf((active[0] == COUNTER_LIFE) ? ">" : " ");
    paint_selector(0u, P1_ROW_LIFE);

    gotoxy(9u, P1_ROW_LIFE);
    print_value(counters[0][COUNTER_LIFE]);

    gotoxy(2u, P1_ROW_DEATHDOOR);
    if (counters[0][COUNTER_LIFE] == LIFE_MIN) {
        printf("  DEATHS DOOR   ");
        paint_row(2u, P1_ROW_DEATHDOOR, 16u, FIRE_PAL);
    } else {
        printf("                ");
        paint_row(2u, P1_ROW_DEATHDOOR, 16u, TEXT_PAL);
    }
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
    paint_selector(P1_COL_CURSOR, p1_element_row[idx]);
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

#define TWO_ROW_HEADER  0u
#define TWO_ROW_LIFE    2u
#define TWO_ROW_DOOR    3u
#define TWO_ROW_ELEM0   5u
#define TWO_ROW_HINT1   11u
#define TWO_ROW_HINT2   12u
#define TWO_ROW_HINT3   13u
#define TWO_ROW_HINT4   14u

static const uint8_t two_life_cursor_col[2] = { 0u, 19u };
static const uint8_t two_life_label_col[2]  = { 1u, 15u };
static const uint8_t two_life_value_col[2]  = { 6u, 11u };
static const uint8_t two_door_col[2]        = { 1u, 15u };
static const uint8_t two_header_col[2]      = { 0u, 15u };
static const uint8_t two_elem_cursor_col[2] = { 0u, 19u };
static const uint8_t two_elem_label_col[2]  = { 1u, 18u };
static const uint8_t two_elem_attr_col[2]   = { 2u, 10u };

// Cursor arrow points toward the middle of the screen on each side -
// right for P1 (left half), left for P2 (right half).
static const char *two_cursor_char[2] = { ">", "<" };

static void two_draw_static_ui(void) {
    uint8_t i;

    gotoxy(1u, TWO_ROW_HINT1);
    printf("SEL:SWAP A+B-");
    gotoxy(1u, TWO_ROW_HINT2);
    printf("START:RESET(cur)");
    gotoxy(1u, TWO_ROW_HINT3);
    printf("SEL+UP:DICE");
    gotoxy(1u, TWO_ROW_HINT4);
    printf("SEL+DN:GLOSS");

    for (i = 0u; i < 4u; i++) {
        paint_row(two_elem_attr_col[0], TWO_ROW_ELEM0 + i, MAX_SYMBOLS, i + 1u);
        paint_row(two_elem_attr_col[1], TWO_ROW_ELEM0 + i, MAX_SYMBOLS, i + 1u);
    }
}

static void two_draw_header(void) {
    gotoxy(two_header_col[0], TWO_ROW_HEADER);
    printf((current_player == 0u) ? "[" : " ");
    paint_selector(two_header_col[0], TWO_ROW_HEADER);
    print_clipped(avatar_codes[avatar_choice[0]], AVATAR_CODE_MAXLEN);
    printf((current_player == 0u) ? "]" : " ");
    paint_selector(two_header_col[0] + 1u + AVATAR_CODE_MAXLEN, TWO_ROW_HEADER);

    gotoxy(two_header_col[1], TWO_ROW_HEADER);
    printf((current_player == 1u) ? "[" : " ");
    paint_selector(two_header_col[1], TWO_ROW_HEADER);
    print_clipped(avatar_codes[avatar_choice[1]], AVATAR_CODE_MAXLEN);
    printf((current_player == 1u) ? "]" : " ");
    paint_selector(two_header_col[1] + 1u + AVATAR_CODE_MAXLEN, TWO_ROW_HEADER);
}

static void two_draw_life(uint8_t p) {
    gotoxy(two_life_cursor_col[p], TWO_ROW_LIFE);
    printf((active[p] == COUNTER_LIFE) ? two_cursor_char[p] : " ");
    paint_selector(two_life_cursor_col[p], TWO_ROW_LIFE);

    gotoxy(two_life_label_col[p], TWO_ROW_LIFE);
    printf("LIFE");

    gotoxy(two_life_value_col[p], TWO_ROW_LIFE);
    print_value(counters[p][COUNTER_LIFE]);

    gotoxy(two_door_col[p], TWO_ROW_DOOR);
    if (counters[p][COUNTER_LIFE] == LIFE_MIN) {
        printf("DOOR");
        paint_row(two_door_col[p], TWO_ROW_DOOR, 4u, FIRE_PAL);
    } else {
        printf("    ");
        paint_row(two_door_col[p], TWO_ROW_DOOR, 4u, TEXT_PAL);
    }
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
    printf((active[p] == COUNTER_AIR + idx) ? two_cursor_char[p] : " ");
    paint_selector(two_elem_cursor_col[p], row);
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

// Redraws the current play state (whichever mode is active) from
// scratch, without touching any counter values. Used both when first
// entering play and when returning to it from a sub-screen (like the
// dice roller) that took over the whole display.
static void redraw_play_screen(void) {
    full_clear();
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

static void enter_play_state(void) {
    current_player = 0u;
    reset_player(0u);
    reset_player(1u);
    game_state = STATE_PLAY;
    redraw_play_screen();
}

static void return_to_title(void) {
    game_state = STATE_TITLE;
    full_clear();
    title_draw();
}

void main(void) {
    uint8_t prev_keys = 0u;
    uint8_t keys, pressed;
    uint8_t a_hold = 0u, b_hold = 0u;

    init_sound();
    prime_console();
    set_bkg_data(TILE_FIRST_ELEMENT, 4u, element_tiles);
    set_bkg_data(TILE_GRID_EMPTY, 2u, grid_square_tiles);
    apply_theme();
    clear_attributes();

    title_draw();

    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        keys = joypad();
        pressed = keys & (uint8_t)(~prev_keys);
        prev_keys = keys;

        if (game_state == STATE_TITLE) {
            if (pressed & J_DOWN) {
                title_selection = (title_selection == 3u) ? 0u : (title_selection + 1u);
                title_draw();
            } else if (pressed & J_UP) {
                title_selection = (title_selection == 0u) ? 3u : (title_selection - 1u);
                title_draw();
            }
            if (pressed & (J_A | J_START)) {
                if (title_selection == 2u) {
                    game_state = STATE_SETTINGS;
                    full_clear();
                    settings_draw();
                } else if (title_selection == 3u) {
                    glossary_return_state = STATE_TITLE;
                    game_state = STATE_GLOSSARY_LIST;
                    full_clear();
                    glossary_list_draw();
                } else if (title_selection == 0u) {
                    // 1-player: it's obviously just "you" - no avatar
                    // select needed, go straight to play.
                    num_players = 1u;
                    enter_play_state();
                } else {
                    num_players = 2u;
                    game_state = STATE_AVATAR_P1;
                    full_clear();
                    avatar_draw(0u);
                }
            }
        } else if (game_state == STATE_SETTINGS) {
            if (pressed & J_LEFT) {
                theme_choice = (theme_choice == 0u) ? (THEME_COUNT - 1u) : (theme_choice - 1u);
                apply_theme();
                settings_draw();
            } else if (pressed & J_RIGHT) {
                theme_choice = (theme_choice == THEME_COUNT - 1u) ? 0u : (theme_choice + 1u);
                apply_theme();
                settings_draw();
            } else if (pressed & (J_B | J_START)) {
                game_state = STATE_TITLE;
                full_clear();
                title_draw();
            }
        } else if (game_state == STATE_AVATAR_P1 || game_state == STATE_AVATAR_P2) {
            uint8_t p = (game_state == STATE_AVATAR_P1) ? 0u : 1u;

            if (pressed & J_LEFT) {
                avatar_choice[p] = (avatar_choice[p] == 0u) ? (AVATAR_COUNT - 1u) : (avatar_choice[p] - 1u);
                avatar_draw(p);
            } else if (pressed & J_RIGHT) {
                avatar_choice[p] = (avatar_choice[p] == AVATAR_COUNT - 1u) ? 0u : (avatar_choice[p] + 1u);
                avatar_draw(p);
            } else if (pressed & (J_A | J_START)) {
                // Avatar select is only ever reached in 2-player mode
                // now (1-player skips straight to play) - so after P1
                // confirms, always go on to P2 next.
                if (p == 0u) {
                    game_state = STATE_AVATAR_P2;
                    full_clear();
                    avatar_draw(1u);
                } else {
                    enter_play_state();
                }
            }
        } else if (game_state == STATE_DICE) {
            if (pressed & J_LEFT) {
                dice_type = (dice_type == 0u) ? (DICE_TYPE_COUNT - 1u) : (dice_type - 1u);
                dice_result = 0u;
                grid_reset();
                dice_draw();
            } else if (pressed & J_RIGHT) {
                dice_type = (dice_type == DICE_TYPE_COUNT - 1u) ? 0u : (dice_type + 1u);
                dice_result = 0u;
                grid_reset();
                dice_draw();
            } else if (pressed & J_A) {
                if (dice_type == DICE_HARBINGER) {
                    roll_grid();
                } else {
                    dice_result = dice_roll(dice_sides[dice_type]);
                }
                dice_draw();
            } else if (pressed & J_B) {
                game_state = STATE_PLAY;
                redraw_play_screen();
            }
        } else if (game_state == STATE_GLOSSARY_LIST) {
            if (pressed & J_DOWN) {
                uint8_t page_count = glossary_page_count(glossary_current_page);
                if (page_count > 0u) {
                    glossary_cursor = (glossary_cursor == page_count - 1u) ? 0u : (glossary_cursor + 1u);
                    glossary_clamp_scroll();
                    glossary_list_draw();
                }
            } else if (pressed & J_UP) {
                uint8_t page_count = glossary_page_count(glossary_current_page);
                if (page_count > 0u) {
                    glossary_cursor = (glossary_cursor == 0u) ? (page_count - 1u) : (glossary_cursor - 1u);
                    glossary_clamp_scroll();
                    glossary_list_draw();
                }
            } else if (pressed & J_LEFT) {
                glossary_current_page = (glossary_current_page == 0u)
                    ? (GLOSSARY_PAGE_COUNT - 1u) : (glossary_current_page - 1u);
                glossary_cursor = 0u;
                glossary_scroll = 0u;
                glossary_list_draw();
            } else if (pressed & J_RIGHT) {
                glossary_current_page = (glossary_current_page == GLOSSARY_PAGE_COUNT - 1u)
                    ? 0u : (glossary_current_page + 1u);
                glossary_cursor = 0u;
                glossary_scroll = 0u;
                glossary_list_draw();
            } else if (pressed & J_A) {
                game_state = STATE_GLOSSARY_DETAIL;
                full_clear();
                glossary_detail_draw();
            } else if (pressed & J_B) {
                if (glossary_return_state == STATE_TITLE) {
                    game_state = STATE_TITLE;
                    full_clear();
                    title_draw();
                } else {
                    game_state = STATE_PLAY;
                    redraw_play_screen();
                }
            }
        } else if (game_state == STATE_GLOSSARY_DETAIL) {
            if (pressed & J_A) {
                uint8_t actual_idx = glossary_page_nth_index(glossary_current_page, glossary_cursor);
                print_status_line("PRINTING...");
                if (printer_print_card(glossary_terms[actual_idx],
                                        glossary_def_lines[actual_idx],
                                        GLOSSARY_MAX_LINES)) {
                    print_status_line("PRINTED!");
                } else {
                    print_status_line("NO PRINTER FOUND");
                }
            } else if (pressed & J_B) {
                game_state = STATE_GLOSSARY_LIST;
                full_clear();
                glossary_list_draw();
            }
        } else {
            // STATE_PLAY. START+SELECT together (in either press order)
            // returns to the title screen; SELECT+UP opens the
            // dice/coin roller; SELECT+DOWN opens the glossary; plain
            // START alone resets the currently focused player.
            if ((pressed & J_START) && (keys & J_SELECT)) {
                return_to_title();
            } else if ((pressed & J_SELECT) && (keys & J_START)) {
                return_to_title();
            } else if ((pressed & J_UP) && (keys & J_SELECT)) {
                game_state = STATE_DICE;
                full_clear();
                dice_draw();
            } else if ((pressed & J_SELECT) && (keys & J_UP)) {
                game_state = STATE_DICE;
                full_clear();
                dice_draw();
            } else if ((pressed & J_DOWN) && (keys & J_SELECT)) {
                glossary_return_state = STATE_PLAY;
                game_state = STATE_GLOSSARY_LIST;
                full_clear();
                glossary_list_draw();
            } else if ((pressed & J_SELECT) && (keys & J_DOWN)) {
                glossary_return_state = STATE_PLAY;
                game_state = STATE_GLOSSARY_LIST;
                full_clear();
                glossary_list_draw();
            } else if (pressed & J_START) {
                reset_player(current_player);
                redraw_life_and_elements_current();
            } else if (num_players == 2u && (pressed & J_SELECT)) {
                current_player = 1u - current_player;
                two_draw_header();
            }

            if (game_state == STATE_PLAY) {
                if (pressed & J_UP) {
                    active[current_player] = (active[current_player] == COUNTER_LIFE)
                        ? (COUNTER_COUNT - 1u) : (active[current_player] - 1u);
                    redraw_life_and_elements_current();
                } else if (pressed & J_DOWN) {
                    active[current_player] = (active[current_player] == COUNTER_COUNT - 1u)
                        ? COUNTER_LIFE : (active[current_player] + 1u);
                    redraw_life_and_elements_current();
                }

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
        }

        vsync();
    }
}
