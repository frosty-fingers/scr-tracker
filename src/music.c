#include "music.h"
#include <gbdk/platform.h>
#include <stdint.h>

// hUGEDriver's public API (from third_party/hUGEDriver/hUGEDriver.h once vendored):
//   extern void hUGE_init(const hUGESong_t *song);
//   extern void hUGE_dosound(void);
// It drives playback from the timer interrupt it installs in hUGE_init,
// so music_update() only needs to be called if you're not using the
// interrupt-driven build of the driver — check hUGEDriver's README for
// which mode its Makefile is set to build.
//
// This file intentionally has no #include "hUGEDriver.h" yet: add it
// once you've vendored the driver (see docs/SETUP.md), along with your
// exported song header, e.g.:
//   #include "hUGEDriver.h"
//   #include "res/music/my_song.h"

static uint8_t music_enabled = 0;

void music_init(void) {
    // hUGE_init(&my_song);
    music_enabled = 1;
}

void music_update(void) {
    if (!music_enabled) return;
    // hUGE_dosound(); // only needed for the polled (non-interrupt) build
}

void music_play(uint8_t song_index) {
    (void)song_index; // extend once multiple songs are wired up
    music_enabled = 1;
}

void music_stop(void) {
    music_enabled = 0;
}
