#include "glossary_logic.h"

uint8_t glossary_page_count_impl(const uint8_t *page_array, uint8_t count, uint8_t page) {
    uint8_t n = 0u;
    uint8_t i;
    for (i = 0u; i < count; i++) {
        if (page_array[i] == page) {
            n++;
        }
    }
    return n;
}

uint8_t glossary_page_nth_index_impl(const uint8_t *page_array, uint8_t count, uint8_t page, uint8_t n) {
    uint8_t i;
    uint8_t seen = 0u;
    for (i = 0u; i < count; i++) {
        if (page_array[i] == page) {
            if (seen == n) {
                return i;
            }
            seen++;
        }
    }
    return 0u;
}
