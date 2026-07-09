#ifndef MUSIC_H
#define MUSIC_H

#include <stdint.h>

// Thin wrapper around hUGEDriver (https://github.com/SuperDisk/hUGEDriver).
// Keeping main.c calling music_init()/music_update() instead of the
// driver's own API means swapping music drivers later only touches
// this one file.
//
// SETUP (see docs/SETUP.md for full steps):
//   1. Vendor hUGEDriver's src/ into third_party/hUGEDriver/
//   2. Export a song from hUGETracker as a .c/.h data file into res/music/
//   3. #include that song header below and point song_data at it

void music_init(void);
void music_update(void);   // call once per frame, e.g. from the main loop
void music_play(uint8_t song_index);
void music_stop(void);

#endif
