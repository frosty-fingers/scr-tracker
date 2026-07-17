// Host-side test for glossary_logic.c - compiled and run with a plain
// compiler (gcc/clang) on whatever machine is doing the testing, NOT
// with the GBDK toolchain. No Game Boy, no emulator - this is testing
// pure logic, not anything hardware-specific. See docs/TESTING.md.
//
// Deliberately uses synthetic test data here instead of #include-ing
// the real src/glossary.h - that keeps these tests independent of
// whatever placeholder/real content happens to be in the glossary at
// any given time, and lets us test edge cases (an empty page, a page
// with only one entry, non-contiguous pages) that the real glossary
// content may not happen to exercise.
//
// Run with: make test   (from the project root)

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../src/glossary_logic.h"

static int failures = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { \
            printf("  ok   - %s\n", msg); \
        } else { \
            printf("  FAIL - %s\n", msg); \
            failures++; \
        } \
    } while (0)

static void test_page_count_mixed(void) {
    // pages: [0, 0, 1, 1, 0]  ->  page 0 has 3 entries, page 1 has 2
    const uint8_t pages[5] = {0u, 0u, 1u, 1u, 0u};
    printf("test_page_count_mixed:\n");
    CHECK(glossary_page_count_impl(pages, 5u, 0u) == 3u, "page 0 count is 3");
    CHECK(glossary_page_count_impl(pages, 5u, 1u) == 2u, "page 1 count is 2");
}

static void test_page_count_empty_page(void) {
    // Every entry is page 0 - page 1 legitimately has zero entries.
    const uint8_t pages[3] = {0u, 0u, 0u};
    printf("test_page_count_empty_page:\n");
    CHECK(glossary_page_count_impl(pages, 3u, 0u) == 3u, "page 0 has all 3 entries");
    CHECK(glossary_page_count_impl(pages, 3u, 1u) == 0u, "page 1 has 0 entries");
}

static void test_page_count_single_entry(void) {
    const uint8_t pages[1] = {0u};
    printf("test_page_count_single_entry:\n");
    CHECK(glossary_page_count_impl(pages, 1u, 0u) == 1u, "single-entry array counts as 1");
}

static void test_nth_index_mixed_non_contiguous(void) {
    // pages: [0, 0, 1, 1, 0]  -  page 0's entries are at real indices
    // 0, 1, 4 (NOT contiguous - index 4 comes after two page-1
    // entries) - this is exactly the case glossary_page_nth_index_impl
    // exists for: resolving "the Nth entry on this page" without
    // assuming entries for a page are grouped together in the array.
    const uint8_t pages[5] = {0u, 0u, 1u, 1u, 0u};
    printf("test_nth_index_mixed_non_contiguous:\n");
    CHECK(glossary_page_nth_index_impl(pages, 5u, 0u, 0u) == 0u, "page 0, 0th entry -> real index 0");
    CHECK(glossary_page_nth_index_impl(pages, 5u, 0u, 1u) == 1u, "page 0, 1st entry -> real index 1");
    CHECK(glossary_page_nth_index_impl(pages, 5u, 0u, 2u) == 4u, "page 0, 2nd entry -> real index 4 (non-contiguous)");
    CHECK(glossary_page_nth_index_impl(pages, 5u, 1u, 0u) == 2u, "page 1, 0th entry -> real index 2");
    CHECK(glossary_page_nth_index_impl(pages, 5u, 1u, 1u) == 3u, "page 1, 1st entry -> real index 3");
}

static void test_nth_index_round_trip(void) {
    // For every (page, n) pair that glossary_page_count_impl says is
    // valid, resolving it with glossary_page_nth_index_impl and then
    // looking up that real index's own page should give back the page
    // we asked for. This is the actual invariant the real code depends
    // on - if this doesn't hold, the glossary list screen would print
    // a different page's entries than the header claims to be showing.
    const uint8_t pages[7] = {1u, 0u, 0u, 1u, 0u, 1u, 1u};
    uint8_t page, n, count, idx;
    printf("test_nth_index_round_trip:\n");
    for (page = 0u; page < 2u; page++) {
        count = glossary_page_count_impl(pages, 7u, page);
        for (n = 0u; n < count; n++) {
            idx = glossary_page_nth_index_impl(pages, 7u, page, n);
            CHECK(idx < 7u, "resolved index is within bounds");
            CHECK(pages[idx] == page, "resolved index actually belongs to the requested page");
        }
    }
}

int main(void) {
    printf("=== glossary_logic tests ===\n");
    test_page_count_mixed();
    test_page_count_empty_page();
    test_page_count_single_entry();
    test_nth_index_mixed_non_contiguous();
    test_nth_index_round_trip();

    printf("===\n");
    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    } else {
        printf("%d check(s) FAILED.\n", failures);
        return 1;
    }
}
