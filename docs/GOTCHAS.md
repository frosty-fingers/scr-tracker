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
