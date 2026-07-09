#include "save.h"
#include <gbdk/platform.h>
#include <gb/gb.h>
#include <string.h>

// SRAM is only mapped into 0xA000-0xBFFF while ENABLE_RAM is set, and the
// hardware only guarantees writes are flushed to the battery cell while
// enabled — so every access is wrapped tightly to minimize the window
// where a reset/power-loss could corrupt data.
static uint8_t *const SRAM_SAVE = (uint8_t *)0xA000;

static uint8_t calc_checksum(const save_data_t *d) {
    uint8_t sum = 0;
    sum += d->version;
    sum += d->player_x;
    sum += d->player_y;
    sum += (uint8_t)(d->score & 0xFF);
    sum += (uint8_t)(d->score >> 8);
    sum += d->flags;
    return sum;
}

uint8_t save_load(save_data_t *out) {
    save_data_t tmp;
    uint8_t ok;

    ENABLE_RAM;
    memcpy(&tmp, SRAM_SAVE, sizeof(save_data_t));
    DISABLE_RAM;

    ok = (tmp.version == SAVE_VERSION) && (tmp.checksum == calc_checksum(&tmp));
    if (ok) {
        memcpy(out, &tmp, sizeof(save_data_t));
    }
    return ok;
}

void save_write(save_data_t *data) {
    data->version  = SAVE_VERSION;
    data->checksum = calc_checksum(data);

    ENABLE_RAM;
    memcpy(SRAM_SAVE, data, sizeof(save_data_t));
    DISABLE_RAM;
}

void save_erase(void) {
    ENABLE_RAM;
    SRAM_SAVE[0] = 0xFF; // invalid version -> save_load() will reject it
    DISABLE_RAM;
}
