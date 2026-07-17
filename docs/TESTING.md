# Testing

Two tiers, because most of this project genuinely can't be tested
without a screen, but some of it can be tested anywhere.

## Tier 1: automated logic tests (run on every push, no hardware needed)

Some of this project's logic is pure - plain array/math operations
with zero dependency on GBDK or Game Boy hardware. That code gets
deliberately extracted into its own module (see `src/glossary_logic.c`
for the first example) so it can be compiled with an ordinary compiler
and tested for real, the same way any regular C project would be.

Run them yourself any time with:

```
make test
```

This also runs automatically on every push, as the very first step in
CI, before the (much slower) GBDK download and ROM build - so a logic
regression fails fast, in seconds, instead of waiting several minutes
for a full build just to find out something's broken.

**What's covered right now:** `glossary_page_count_impl()` and
`glossary_page_nth_index_impl()` (the glossary's page-navigation math)
- see `tests/test_glossary_logic.c`.

**What's NOT covered, and can't be with this approach:** anything that
actually touches hardware - VRAM writes, tile rendering, controller
input, the printer protocol, sound. That's what Tier 2 is for.

**Extending this**: when new pure logic gets written (bounds-checking,
indexing, anything that's "just C" with no `gb/gb.h` calls in it), pull
it into its own `src/*_logic.c`/`.h` pair like `glossary_logic`, add a
`tests/test_*.c` file for it, and add it to the `test` target in the
Makefile. The existing test file is a reasonable template to copy.

## Tier 2: manual regression checklist (needs the actual device/emulator)

Everything below needs eyes on a real screen. Work through it after
any build that touches more than a trivial one-line change - checking
this list top-to-bottom takes a few minutes and would have caught
several of the "unrelated feature broke" bugs found so far.

### Title screen
- [ ] Boots straight to the title screen, no garbage/blank screen.
- [ ] UP/DOWN cycles through all 4 options (1 PLAYER / 2 PLAYER /
      SETTINGS / GLOSSARY) with wraparound both directions.
- [ ] Cursor (`>`) and its accent color track the selected option.
- [ ] Version number shows at the bottom (`V<number>` from CI, or
      `Vlocal` from a local build).

### Settings / themes
- [ ] Enter from title, LEFT/RIGHT cycles all 3 themes (GREY,
      PARCHMENT, MIDNIGHT) with a live preview - the whole screen
      recolors immediately, not just on confirm.
- [ ] B returns to the title screen with the chosen theme still active.
- [ ] Re-entering SETTINGS later still shows the previously chosen
      theme (persists for the play session, not saved across power-off
      - that's expected, not a bug).

### 1-player mode
- [ ] Selecting "1 PLAYER" goes straight into play - no avatar select
      screen (that's intentional).
- [ ] UP/DOWN moves the cursor between LIFE and the 4 elements, with
      wraparound.
- [ ] A/B change the selected counter by 1; holding auto-repeats after
      a short delay.
- [ ] LIFE clamps at 20 (top) and 0 (bottom) - can't go outside that
      range.
- [ ] At LIFE 0: "DEATHS DOOR" appears in the theme's FIRE color, and
      A/B stop doing anything to LIFE until reset.
- [ ] START resets LIFE to 20 and all elements to 0.
- [ ] Element counts show as that many repeated symbols (not numbers)
      up to 8, with the exact count printed alongside past 8.
- [ ] Each element's symbol reads in its own distinct color (not all
      the same, not blending into the background).

### 2-player mode
- [ ] Selecting "2 PLAYER" goes to avatar select for P1, then P2, then
      into play.
- [ ] Screen splits left (P1) / right (P2), P2's layout mirrored -
      cursor points left, symbols grow toward the middle.
- [ ] SELECT swaps which player UP/DOWN/A/B/START currently control -
      header brackets (`[XXX]`) track whose turn it is.
- [ ] START only resets the *focused* player, not both.
- [ ] Each player's Death's Door triggers/clears independently.

### Dice / coin roller (SELECT+UP during a match)
- [ ] Opens from both 1P and 2P without disturbing the underlying game
      state (life/element values unchanged when you back out).
- [ ] LEFT/RIGHT cycles all 4 types: D20, D6, COIN, HARBINGER.
- [ ] D20/D6 give a number in the right range; COIN gives HEADS/TAILS.
- [ ] HARBINGER shows a 5x4 grid, exactly 3 squares highlighted per
      roll, never duplicates, never fewer/more than 3.
- [ ] Switching types clears the previous type's result/grid cleanly -
      no leftover digits, no leftover highlighted squares, no leftover
      color bleeding into new text on the same rows.
- [ ] B returns to the exact game state you left.

### Glossary (GLOSSARY from title, or SELECT+DOWN during a match)
- [ ] LEFT/RIGHT switches between KEYWORDS and TOKENS pages, cursor
      and scroll reset cleanly on every switch.
- [ ] UP/DOWN scrolls the current page's list, wraparound at both ends.
- [ ] A opens a term's definition; B returns to the list; B again
      returns to wherever the glossary was opened from (title screen
      or back into the match, correctly, not mixed up).
- [ ] (If a printer is ever actually available) A on a definition
      attempts to print it - see docs/PRINTER.md, this part is
      unverified regardless of what this checklist says.

### Cross-cutting (things that broke before specifically because they
cut across multiple screens - worth re-checking after almost any change)
- [ ] START+SELECT together, from anywhere in a match, returns to the
      title screen (works regardless of which button's press triggers
      it).
- [ ] No leftover color from one screen bleeds into text on a
      different screen after switching between them a few times in a
      row (this exact class of bug has recurred more than once - see
      docs/GOTCHAS.md's cls()/attribute-plane entries).
- [ ] Play through a full loop: title -> a mode -> dice roller ->
      glossary -> back to title -> the other mode. Nothing should look
      wrong at any transition.

## How to help

The most useful thing: actually running the Tier 2 checklist above
after a build and reporting back which lines failed (or all-clear) -
that's the one part of this that genuinely needs your hardware/eyes,
nobody else can do it. Doesn't need to be the whole list every time -
even just the sections near whatever changed is a big improvement over
the current "test whatever seems relevant" approach.

If you want to help with Tier 1 too: flagging which pieces of logic
feel highest-risk or most worth locking down with a test next (dice
roll ranges, the Harbinger grid's "always exactly 3 unique squares"
property, and the printer's packet checksum math are reasonable
next candidates) would help prioritize what gets extracted and tested
next, versus guessing at it in isolation.
