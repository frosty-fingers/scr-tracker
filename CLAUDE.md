# Project instructions for Claude

This is a Game Boy Color homebrew game built with GBDK-2020 (C, not assembly).
Read this before making changes.

## Constraints Claude should always respect

- **Target hardware is a Game Boy Color**, not a modern machine. CPU is ~4MHz
  z80-like (sm83), 8KB WRAM, 8KB VRAM (x2 banks on GBC), no floating point
  in hot paths, no dynamic allocation in gameplay code (no malloc in the
  main loop — static/global buffers only).
- **Cartridge config is fixed** in the Makefile: MBC5+RAM+BATTERY
  (`CART_TYPE := 0x1B`), CGB-compatible mode (`-Wm-yc`). Don't change these
  without explicitly flagging it — they affect save compatibility.
- **GBDK-2020 API, not raw hardware registers**, unless there's a specific
  reason (e.g. timing-critical HBlank code). Prefer `set_sprite_tile()`,
  `move_sprite()`, etc. over direct register pokes.
- **SRAM access must stay wrapped in `ENABLE_RAM`/`DISABLE_RAM`**, and that
  window should be as short as possible — see `src/save.c` for the pattern.
  Never leave SRAM enabled across a frame boundary.
- **Bump `SAVE_VERSION` in `src/save.h`** any time the layout of
  `save_data_t` changes, so old saves get rejected instead of misread.
- **ROM bank budget matters.** Without banking, code + data must fit in
  32KB. Flag it if a change is likely to blow that budget so we can discuss
  enabling banking/autobank rather than silently failing at link time.

## Workflow (I'm developing from a phone — no local compiler)

- I can't compile locally. Every change needs to go through GitHub Actions
  (`.github/workflows/build-rom.yml`) to produce a playable `.gbc`.
- After making changes, tell me clearly that I need to **commit and push**
  to trigger the build, and remind me where to download the artifact
  (Actions tab → latest run → Artifacts).
- I test ROMs on **Delta** (iOS) — mention if a change needs testing on
  real hardware or a more accurate emulator (BGB/Emulicious) instead,
  e.g. anything touching cart timing, save RAM, or CGB palette edge cases.
- Since I can't run a debugger locally, prefer changes that fail loudly
  (visible on-screen state, not silent freezes) and explain how to verify
  a fix worked just by playing the ROM.

## Code style

- Keep functions small and named for what they do on-screen (e.g.
  `move_player`, `draw_hud`) — I'm learning C alongside this project, so
  favor readability over cleverness.
- Comment *why*, not *what*, especially around GBDK quirks or hardware
  gotchas (timing windows, bank switches, VRAM access restrictions) —
  those aren't obvious coming from other languages.
- New source files go in `src/`; new art in `res/gfx/`; new music in
  `res/music/`. Update the Makefile's `assets` target when adding new
  PNGs to convert.

## When something's ambiguous

Ask before making structural changes (new MBC type, banking scheme,
save format) — but for straightforward gameplay/bugfix work, just make
a reasonable call and explain it briefly rather than stopping to ask.

## Session handoff — read this first

I use Claude across separate chats, so there's no guaranteed memory
between sessions. **Before doing anything else, read `docs/STATUS.md`**
if it exists — it holds current task, what's confirmed working, what's
in progress/blocked, and the next step. Treat it as more current than
your own assumptions about project state.

**Keep `docs/STATUS.md` up to date as you go**, not just at the end:
- Update it after any change that alters what's confirmed working vs.
  still broken/untested (e.g. after isolating a bug, after confirming a
  fix from a screenshot).
- If a conversation is getting long or you sense you're running low on
  room to keep working, proactively rewrite `docs/STATUS.md` to reflect
  the exact current state *before* that becomes a problem — don't wait
  to be asked. A stale status file is worse than no status file.
- When you update it, say so briefly (e.g. "updated docs/STATUS.md")
  so I know to commit it along with the code.

Format: current focus, confirmed working, in progress/blocked (with
what's been ruled out and why), next step. Keep it short — a few
paragraphs, not a full log. `docs/GOTCHAS.md` stays the permanent
record of confirmed hardware/toolchain issues; `docs/STATUS.md` is the
disposable "what were we doing" snapshot that gets rewritten each time,
not appended to.
