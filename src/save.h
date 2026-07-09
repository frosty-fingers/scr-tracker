#ifndef SAVE_H
#define SAVE_H

#include <stdint.h>

// Bump this whenever the layout of save_data_t changes.
// save_load() rejects saves whose version doesn't match, rather than
// risk interpreting stale/foreign bytes as valid game state.
#define SAVE_VERSION 1

typedef struct {
    uint8_t  version;
    uint8_t  player_x;
    uint8_t  player_y;
    uint16_t score;
    uint8_t  flags;       // bitfield: story flags, unlocks, etc.
    uint8_t  checksum;    // simple additive checksum over the fields above
} save_data_t;

// Returns 1 if a valid save was found and loaded into `out`, 0 otherwise.
uint8_t save_load(save_data_t *out);

// Writes `data` to battery-backed SRAM (bank 0), stamping version + checksum.
void save_write(save_data_t *data);

// Erases the save slot (writes an invalid version byte).
void save_erase(void);

#endif
