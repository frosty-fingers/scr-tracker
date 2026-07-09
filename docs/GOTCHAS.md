# Gotchas

Confirmed-on-real-device (or real emulator) issues and their fixes. Add to
this as new ones turn up — the point is to never debug the same thing twice.

## CGB palette must be set explicitly, or screen renders blank/white

**Symptom:** ROM builds and runs (no crash), but the screen shows solid
white — no visible sprites, background, or text — on Delta (iOS) and
presumably other CGB-aware emulators/hardware.

**Cause:** Building with `-Wm-yc` (CGB-compatible mode) only sets a header
flag; it doesn't configure a color palette. Left unset, the hardware/emulator
can end up rendering foreground and background as the same color —
effectively invisible — rather than falling back to a sane default.

**Fix:** Force the classic grayscale palette explicitly at startup, before
enabling the display:

\`\`\`c
#include <gb/cgb.h>

if (_cpu == CGB_TYPE) {
    set_default_palette();
}
\`\`\`

**Dead end that looked plausible but wasn't:** targeting the platform with
`-msm83:gbc` in LCCFLAGS. GBDK's Game Boy family only has one platform
(`gb`) — color mode is controlled purely by the `-Wm-yc`/`-Wm-yC` header
flags, not by `-m`. Passing `gbc` as a platform name fails at compile time
with `unrecognized PORT:PLATFORM`.

**Confirmed:** 2026-07-09, GBDK-2020 4.5.0, tested on Delta (iOS), via the
CI build in `.github/workflows/build-rom.yml`.

## music.h missing stdint.h caused a syntax error in music.c

**Symptom:** `make` failed compiling `src/music.c` with a syntax error
pointing at a parameter name in `music.h`, even though `main.c` (which
also includes `music.h`) compiled fine.

**Cause:** `music.h` used `uint8_t` without including `<stdint.h>` itself.
It happened to work in `main.c` because other headers pulled in
`<stdint.h>` first — but `music.c` includes `"music.h"` before
`<stdint.h>`, so `uint8_t` was undefined at that point.

**Fix:** Every header should include its own dependencies rather than
relying on include order. Added `#include <stdint.h>` to the top of
`music.h` (already applied in this repo).

**Confirmed:** 2026-07-09, GBDK-2020 4.5.0, via CI build.

## tools/release.sh needs to be run via `bash`, not directly

**Symptom:** `make release` failed with `Permission denied` on
`./tools/release.sh` in CI, right after a successful build.

**Cause:** The file was created through GitHub's web editor, which
doesn't preserve the executable bit — there's no way to `chmod +x` a
file from that UI, only from a real git client.

**Fix:** Call it as `bash tools/release.sh` in the Makefile instead of
`./tools/release.sh`, so it doesn't depend on the executable bit at all
(already applied in this repo's Makefile).

**Confirmed:** 2026-07-09, via CI build.

