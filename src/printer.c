#include <gb/gb.h>
#include <stdint.h>

#include "printer.h"

// See printer.h for the overall confidence/safety notes. This file is
// the actual protocol implementation - GB Printer packet framing on
// top of GBDK's send_byte()/receive_byte() (see gb/gb.h: _io_out,
// _io_in, _io_status, IO_IDLE/IO_SENDING/IO_RECEIVING/IO_ERROR).

#define PRT_CMD_INIT   0x01u
#define PRT_CMD_PRINT  0x02u
#define PRT_CMD_DATA   0x04u

// Bounded polling limit for each individual byte transfer - not a real
// timer, just a hard ceiling so a wait loop can never run forever. In
// practice send_byte()/receive_byte() run on the Game Boy's own
// internal clock (this device is always the "master" when talking to
// a printer), so they complete in a few thousand CPU cycles regardless
// of whether anything is actually connected - this limit is a safety
// net, not something expected to normally get hit.
#define PRT_MAX_WAIT  4000u

// Printer accepts up to 0x280 (640) data bytes per packet (per the
// documented protocol) before requiring a new packet - stay well under
// that. 20 tiles * 16 bytes/tile = 320 bytes per chunk.
#define PRT_CHUNK_TILES  20u

#define CARD_WIDTH     20u
#define CARD_MAX_ROWS  7u  // 1 title row + up to 6 definition lines

static uint8_t printer_send(uint8_t b) {
    uint16_t i;
    _io_out = b;
    send_byte();
    for (i = 0u; i < PRT_MAX_WAIT; i++) {
        if (_io_status != IO_SENDING) {
            break;
        }
    }
    return (_io_status == IO_IDLE);
}

static uint8_t printer_recv(uint8_t *out) {
    uint16_t i;
    receive_byte();
    for (i = 0u; i < PRT_MAX_WAIT; i++) {
        if (_io_status != IO_RECEIVING) {
            break;
        }
    }
    if (_io_status == IO_IDLE) {
        *out = _io_in;
        return 1u;
    }
    return 0u;
}

// Sends one full framed packet (magic bytes, command, compression=off,
// length, data, checksum) and reads back the 2-byte status handshake
// that follows every packet. Returns 1 if every byte transfer in the
// exchange completed, 0 if any single one failed/timed out - this is
// about transport-level success, not about whether the *content* of
// the response indicates a real printer is there (see
// printer_is_present() for the one place that distinction matters).
static uint8_t printer_send_packet(uint8_t cmd, const uint8_t *data, uint16_t len, uint8_t *status_out) {
    uint16_t checksum;
    uint16_t i;
    uint8_t b1, b2;

    if (!printer_send(0x88u)) { return 0u; }
    if (!printer_send(0x33u)) { return 0u; }
    if (!printer_send(cmd))   { return 0u; }
    if (!printer_send(0x00u)) { return 0u; }  // compression: off
    if (!printer_send((uint8_t)(len & 0xFFu)))         { return 0u; }
    if (!printer_send((uint8_t)((len >> 8u) & 0xFFu))) { return 0u; }

    checksum = (uint16_t)cmd + (uint16_t)(len & 0xFFu) + (uint16_t)((len >> 8u) & 0xFFu);
    for (i = 0u; i < len; i++) {
        if (!printer_send(data[i])) {
            return 0u;
        }
        checksum += data[i];
    }

    if (!printer_send((uint8_t)(checksum & 0xFFu)))         { return 0u; }
    if (!printer_send((uint8_t)((checksum >> 8u) & 0xFFu))) { return 0u; }

    // Two more bytes out (keepalive), two bytes back - the second is
    // the printer's status byte.
    if (!printer_send(0x00u)) { return 0u; }
    if (!printer_send(0x00u)) { return 0u; }
    if (!printer_recv(&b1)) { return 0u; }
    if (!printer_recv(&b2)) { return 0u; }

    if (status_out != 0) {
        *status_out = b2;
    }
    return 1u;
}

uint8_t printer_is_present(void) {
    uint8_t status = 0xFFu;

    if (!printer_send_packet(PRT_CMD_INIT, 0, 0u, &status)) {
        return 0u;
    }

    // An unconnected link cable line floats high, so a read with
    // nothing on the other end tends to come back 0xFF rather than a
    // real status byte - treat that as "no printer", rather than
    // trusting that the exchange merely *completing* means something
    // is there (send_byte()/receive_byte() run on this device's own
    // clock and complete on their own timing either way). This
    // heuristic is the least-verified part of the whole feature - if
    // presence detection is ever wrong, this is the first place to
    // look.
    return (status != 0xFFu);
}

// Fills one CARD_WIDTH-wide row of font tile IDs from a C string,
// padding the remainder with tile 0 (space) once the string ends -
// same "always clear the full field" reasoning as everywhere else text
// gets redrawn in this project (see docs/GOTCHAS.md). Uses the same
// char-to-tile mapping the console font itself uses (tile ID = ASCII
// code - 32), so this only works correctly for characters actually in
// the standard font range - true for everything this project ever
// prints (uppercase letters, digits, space, basic punctuation).
static void card_fill_row(uint8_t *row_tiles, const char *text) {
    uint8_t i;
    uint8_t c;

    for (i = 0u; i < CARD_WIDTH; i++) {
        c = (uint8_t)text[i];
        if (c == 0u) {
            for (; i < CARD_WIDTH; i++) {
                row_tiles[i] = 0u;
            }
            return;
        }
        row_tiles[i] = (uint8_t)(c - 32u);
    }
}

uint8_t printer_print_card(const char *title, const char **lines, uint8_t max_lines) {
    uint8_t card_tiles[CARD_MAX_ROWS][CARD_WIDTH];
    uint8_t row_count;
    uint8_t r, c, i;
    uint8_t status;
    uint8_t pattern[16];
    uint8_t chunk[PRT_CHUNK_TILES * 16u];
    uint16_t chunk_len;
    uint8_t tiles_in_chunk;
    uint16_t total_tiles;
    uint16_t tile_index;
    uint8_t print_params[4];

    if (!printer_is_present()) {
        return 0u;
    }

    card_fill_row(card_tiles[0], title);
    row_count = 1u;
    for (r = 0u; r < max_lines && row_count < CARD_MAX_ROWS; r++) {
        card_fill_row(card_tiles[row_count], lines[r]);
        row_count++;
    }
    for (; row_count < CARD_MAX_ROWS; row_count++) {
        for (c = 0u; c < CARD_WIDTH; c++) {
            card_tiles[row_count][c] = 0u;
        }
    }

    if (!printer_send_packet(PRT_CMD_INIT, 0, 0u, &status)) {
        return 0u;
    }

    total_tiles = (uint16_t)CARD_MAX_ROWS * CARD_WIDTH;
    tile_index = 0u;
    while (tile_index < total_tiles) {
        tiles_in_chunk = 0u;
        chunk_len = 0u;
        while (tiles_in_chunk < PRT_CHUNK_TILES && tile_index < total_tiles) {
            r = (uint8_t)(tile_index / CARD_WIDTH);
            c = (uint8_t)(tile_index % CARD_WIDTH);
            get_bkg_data(card_tiles[r][c], 1u, pattern);
            for (i = 0u; i < 16u; i++) {
                chunk[chunk_len + i] = pattern[i];
            }
            chunk_len += 16u;
            tiles_in_chunk++;
            tile_index++;
        }
        if (!printer_send_packet(PRT_CMD_DATA, chunk, chunk_len, &status)) {
            return 0u;
        }
    }

    // An empty data packet is required immediately before the print
    // command, or the print command is ignored (documented protocol
    // quirk, not something worked out by trial and error here).
    if (!printer_send_packet(PRT_CMD_DATA, 0, 0u, &status)) {
        return 0u;
    }

    print_params[0] = 1u;     // sheet feed (1 = normal)
    print_params[1] = 0x00u;  // margins: none before/after
    print_params[2] = 0xE4u;  // standard 4-shade palette mapping
    print_params[3] = 0x40u;  // exposure/darkness: documented default
    if (!printer_send_packet(PRT_CMD_PRINT, print_params, 4u, &status)) {
        return 0u;
    }

    return 1u;
}
