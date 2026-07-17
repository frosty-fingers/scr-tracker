#ifndef PRINTER_H
#define PRINTER_H

#include <stdint.h>

// Game Boy Printer support - UNTESTED against real or emulated printer
// hardware (no way to test this - see docs/GOTCHAS.md and
// docs/PRINTER.md for the full writeup of what's confirmed vs assumed).
//
// The low-level transport (send_byte()/receive_byte()/_io_out/_io_in/
// _io_status) is GBDK's own documented, code-example-verified async
// serial I/O API - high confidence there. The GB Printer *packet
// protocol* built on top of it (magic bytes, command/checksum framing,
// print parameters) is reasoned from the community-documented protocol
// (gbdev wiki), which is consistent across many independent
// implementations - reasonable confidence, but genuinely unverified.
//
// Safety-first design: every wait loop is bounded by a hard iteration
// count, never an unbounded while-loop - if a printer isn't connected
// or something about the protocol assumptions is wrong, this fails
// closed (reports "not connected"/gives up) rather than ever hanging
// the game. That property matters more than protocol perfection here.

// Returns 1 if a printer responds to a status query, 0 otherwise
// (either no device connected, or an unexpected response). Safe to
// call any time - always returns within a bounded number of attempts,
// never blocks indefinitely.
uint8_t printer_is_present(void);

// Prints a "card": a title line plus up to max_lines lines of text,
// each line printed as-is (no wrapping - same convention as
// glossary_def_lines). Renders using whatever's currently loaded in
// VRAM for the standard console font (so this only works correctly for
// characters already on screen somewhere - true for anything drawn via
// printf(), which is everything in this project). Returns 1 on
// apparent success, 0 on failure (including "no printer present").
uint8_t printer_print_card(const char *title, const char **lines, uint8_t max_lines);

#endif // PRINTER_H
