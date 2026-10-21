# ENDING.EXE recovery

All 175 identified game/engine functions are matching C as of 2026-10-21:
42,760 bytes at `0x800138c0..0x8001dfc8`. There are no game assembly fallbacks.
The complete 264,192-byte executable remains identical to retail, including
the preserved SDK and data:

```text
b7ada48dbc9d78098bd2e9fbf8589bdbcb1c64d963abc6282cd65855bf2bb606
```

The fifteen combined modules are recorded in
[`translation-units.ending.exe.json`](../config/translation-units.ending.exe.json).
Standalone archive, CD-file, debug-display, font, and card helpers reuse the
existing implementations. Source coverage comes from that manifest and its
includes, rather than counting generated stubs as C.

## Reproduce the checks

Run these from `nix develop`, or prefix each command with
`nix develop --command`:

```sh
./Build -j8 check-ending.exe
./Build -j8 check-all
python tools/progress.py --target ending.exe
python tools/menu_data_inventory.py --target ending.exe --check
python -m unittest tools.tests.test_source_units tools.tests.test_shared_symbols tools.tests.test_target_inventory tools.tests.test_menu_data_inventory
```

All six image checks and the 37 focused tests pass. The data tests cover both
MENU and ENDING, including corrupted switch consumers/targets, missing voice
sentinels, changed audio cues, and nonzero bytes hidden in padding/reserved
storage. Full-image equality is the matching oracle; no new emulator behavior
claim is needed to establish identical machine code.

## Shared code and ENDING differences

Seven complete modules reuse MENU's exact implementations: VALLOC, 3DCTRL,
FILEIO, IMAGES, OPAUDIO, EXECFILE, and MEMCARD. MEMCARD includes the shared
MAIN implementation with ENDING's own string definitions, because MENU's
unused save-version diagnostic is absent here. Shared code was accepted only
when every instruction agreed apart from actual object relocation operands.
`tools/shared_symbols.py` now accepts `--source menu.exe`, for example:

```sh
python tools/shared_symbols.py FILEIO --source menu.exe --target ending.exe --at 0x80015d74
```

That complete-module comparison reconciles 75 symbol placements. Constants,
register choices, branch order, and member offsets remain strict. A source
executable must first match its retail image. The destination is then compiled
and checked as a whole.

The remaining modules preserve these observed differences:

- PADCMD uses the earlier 12-byte pad record. Its decoder fills the actuator
  buffer using `PadInfoMode`, unlike MENU; it lacks MAIN's extra active flag.
  The complete `ComPad` is 472 bytes, versus MENU's 420 and MAIN's 480.
- OPMOVIE includes the extra 40-byte `SyncMovieFrame` wrapper. Its other ten
  stream/decode helpers match MENU.
- AUDIO's `FindSETone` searches program/tone without MENU's mode-4 filter.
  The two playback pitches retain independent voice counters.
- FRONTEND uses held Circle/Cross/Square to skip its generic fade/movie loops,
  then waits for release. Its movie loop uses `StartDrawing`/`EndDrawing`.
- SEMNG uses the fixed `SOUND\\STAGE` development path, without MENU's
  language-selected prefix.
- SUBTITLE interprets voice, caption, and frame-wait CAD records. Stage 12
  selects the player's epilogue captions; stage 13 uses the shared Rikimaru
  caption file. The voice-bank binding is global here.
- MAIN unlocks the extra menu languages, plays the character epilogue for
  627 frames, then credits for 5,295 frames. Only credits accept Start/Circle
  skipping. It selects the localized `ENDR`, `ENDA`, or `ENDC` voice bank.
- RESULTS compares current and stored mission score components, draws the
  rank and the Manebue reward icon for Grand Master, and handles return/retry.
  Its enabled save flow sets the chosen character's maximum campaign uid to
  `STAGE_UID_FREE_PRINCESS` before entering the shared card UI.

The persistent handoff remains at `TENCHU_PERSISTENT_STATE_ADDRESS`; ENDING
uses the same `TLinkInfo` layout as MAIN/MENU. Ordinary named symbols still
represent ENDING-owned storage. GP flags in `Build.hs` describe original
translation-unit ownership, including the subtitle globals and the shared
movie/results button state; they are not function-specific compiler options.

`main` retains 32 unaccessed stack bytes before its rectangle. Their original
type and purpose are unknown. The save input loop also preserves retail's
uninitialized first held-button value. No invented initialization changes
either behavior.

## Game/SDK boundary

`AdtSelect` ends at `0x8001dfc8`. The next three complete 16-byte spans equal
MENU's known BIOS thunks byte for byte:

| ENDING address | MENU address | Entry |
|---|---|---|
| `0x8001dfc8` | `0x80027824` | Exec |
| `0x8001dfd8` | `0x80027834` | Krom2RawAdd |
| `0x8001dfe8` | `0x80027844` | ChangeClearPAD |

CRT entry is `0x8001e06c`; its call at `0x8001e108` reaches ENDING's `main`
at `0x8001b56c`. Scanning direct J/JAL targets throughout
`0x8001dfc8..0x80047898` finds this as the only transfer back into the game
span. These independent checks establish the boundary used by
`tools/target_inventory.py`.

The SDK/CRT span is 170,192 bytes across 643 scaffold rows, all preserved as
assembly. Several rows contain multiple library functions, so 643 is an
inventory-row count, not a claim of complete SDK function discovery. The
whole executable has 818 identified rows and 212,952 inventoried code bytes.

## Leading data

[`ending-leading-data.tsv`](../reference/ending-leading-data.tsv) accounts for
all 10,432 bytes at `0x80011000..0x800138c0`. The shared inventory tool verifies
the leading-data hash, complete nonoverlapping coverage, switch consumers'
address construction and branch destinations, audio records, and sentinels.

| Classification | Bytes |
|---|---:|
| ASCII strings | 5,024 |
| Shift-JIS save title | 21 |
| Music records | 228 |
| Voice records | 1,944 |
| Voice-table pointers | 16 |
| Sixteen switch tables | 2,112 |
| Zero fill | 1,087 |

C owns 6,120 leading bytes: engine and localized strings, typed IMAGES audio
data, and the two card-screen switch tables. The IMAGES tables share MENU's
exact layout, including nineteen music records, 324 voice records with
sentinels, 23 relocatable pointers, and the 504 unexplained zero bytes.

Raw textbin totals 4,312 bytes. Of these, 4,304 are SDK data at
`0x800127f0..0x800138c0`. The other eight are zero fill at
`0x800121bc..0x800121c0` and `0x800125ac..0x800125b0`; their original
declarations remain unknown. Classifying a byte does not claim its original
C declaration has been recovered. Loaded global data after the code remains
preserved by the existing data scaffold.

## Matching lessons

Most final routines compiled exactly from the shared source shapes on the
first pass. The results screen required a real source distinction: take its
current-statistics pointer during graphics setup, and use the persistent
struct's language field when loading the two localized resources. That keeps
the loop's statistics base and the setup's persistent base separate, matching
the original address reuse and register allocation.

The save loop initializes `pressed` after polling the pad. Moving that
assignment before the poll produces the same behavior but substitutes the
saved zero-valued register for `$zero` in the call argument. Scheduling can
move the final initialization earlier after common-subexpression elimination;
its machine-code position does not prove the assignment preceded the poll in C.

After symbol renames, splat can retain generated SDK assembly referring to an
old `D_` alias. Remove only the stale generated stub/assembly and the target's
`.done` file, then regenerate. Do not add redundant linker aliases to conceal
the stale input. This occurred for the SDK user of `StCdIntrFlag`.
