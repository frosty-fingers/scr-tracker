#ifndef GLOSSARY_LOGIC_H
#define GLOSSARY_LOGIC_H

#include <stdint.h>

// Pure page-navigation math for the glossary - deliberately has ZERO
// dependency on GBDK/hardware headers (no gb/gb.h, no VRAM calls,
// nothing that only exists on a Game Boy). That's what lets this same
// code be compiled two different ways:
//   1. As part of the real ROM (src/main.c calls these functions).
//   2. As part of a plain host-side test build (see tests/), using an
//      ordinary compiler (gcc) on any machine, with no emulator and no
//      Game Boy needed - real automated regression tests that can run
//      in CI on every push, not just a manual checklist.
// See docs/TESTING.md for the whole testing strategy this fits into.

// How many entries in a glossary_page[] array of the given length
// belong to the given page.
uint8_t glossary_page_count_impl(const uint8_t *page_array, uint8_t count, uint8_t page);

// Resolves a page-relative position (the "nth entry on this page", 0-
// based) to its real index into the original array. Entries for a page
// don't need to be contiguous, so this always does a fresh scan rather
// than assuming any particular layout. Returns 0 if n is out of range
// for that page (shouldn't happen if the caller checks
// glossary_page_count_impl() first, but this can't fail loudly on a
// value it doesn't own by throwing/crashing, only by returning
// *something* - see docs/TESTING.md's notes on this function's tests
// for why 0 specifically, and what a caller should do to detect it
// properly instead of trusting the return value on its own).
uint8_t glossary_page_nth_index_impl(const uint8_t *page_array, uint8_t count, uint8_t page, uint8_t n);

#endif // GLOSSARY_LOGIC_H