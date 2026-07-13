# Tenchu (PS1) decompilation — build plan

## Approach

Split the original `main.exe` into sections and per-function ASM with `splat`
(`split.py`), then progressively replace `INCLUDE_ASM` stubs with C that
recompiles to the same bytes. The build (Haskell **Shake**, `shake/src/Build.hs`,
which superseded the old `Makefile`) reassembles everything and gates on a
byte-identical `main.exe`.

## Status ✅

- **Round-trip byte-matches.** `./Build clean && ./Build check` reassembles a
  **byte-identical** `main.exe` (sha256 `0690a5c1…3558`) from a clean state.
- **Toolchain is complete for matching.** Pipeline is
  `cpp | cc1-281 -G8 | maspsx --aspsx-version=2.77 | as | ld`; reproducible/offline
  via nix (no cabal/wine). maspsx is integrated (see [`docs/toolchain.md`](docs/toolchain.md)).
- **Every game function is carved (555/555)**; matched count is live in
  `tools/progress.py` (206/555 game functions, 14.1% of game-code bytes at the time
  of writing), whole-image byte-identical throughout (see `git log` for the
  per-function record; `tools/progress.py` for the live count). The engine is
  the matcher-agent pipeline ([`docs/orchestration.md`](docs/orchestration.md))
  driven **family-first**: map a subsystem's shared struct ONCE and clone/reuse
  across the whole family — `src/main.exe/item.h`
  (`PARAM_ITEM_USE`/`Humanoid`/`MotionManager`/`MotionRegistType`) and
  `src/main.exe/game_types.h` (the byte-proven `character_state`). Highest-yield
  seams landed: the `ProcItem*`/`ReqItem*` item TU, the CD/AFS file-access
  wrappers, the character-AI `Think*`/`think_setting_*`/`character_state`
  family, and small render/util helpers. **The clean seam** (non-jump-table,
  ≤~500-byte, non-dispatch functions) matches **4–5/5 per batch at ~65–250k
  tokens** on Sonnet; pick targets with `tools/triage.py` / `tools/findsimilar.py`.
- **Partial matches** (blocked on sub-C-level residuals, kept via the
  NON_MATCHING convention — default build stays green byte-identical, draft
  builds with `NON_MATCHING=<Name> ./Build`): **~30 functions parked**, each
  root-caused in its file header. Residuals are consistently sub-C —
  register-allocation / scheduling / reload-pass ties the source can't steer:
  the named **`la` address-materialization tie** (`%hi` in a temp vs the target
  reg — `PrepareAccess`, `cd_open`, `PlayMusicFromID`, `FUN_8004a368`),
  goto-merge copy-chains (`turn_towards_player_`, `Think3chase`), and the
  big-handler flag/frame ties (`handle_char_state_*`/`Think3*` dispatchers,
  ~1400–1700 bytes — the **tie-prone bucket**). The permuter is *provably immune*
  to the reload-tie class, so these are parked (the sub-C early-stop protocol),
  not burned on. [`docs/matching-cookbook.md`](docs/matching-cookbook.md)
  records ~60 verified matching idioms. The early sessions pinned down the real
  gp model (ASPSX gp-addresses only TU-local definitions; externs are absolute)
  and produced the reusable infrastructure in
  [`docs/toolchain.md`](docs/toolchain.md): the opt-in `maspsx --gp-extern`
  patch + per-file lists in `Build.hs` (`maspsxGpExterns`), and the `macro.inc`
  `li→addiu` fix. The compiler is the canonical decompals `gcc-2.8.1-psx`,
  nix-pinned and verified equivalent to real Sony `CC1PSX.EXE` (decomp.me
  `psyq4.3`) — see `compiler-fidelity` in the toolchain doc.
- **Modding + emulator loop works:** `./Build mod` (grow functions) and
  `./Build iso`/`iso-mod` (bootable disc for pcsx-redux). See docs/.
- **decomp.dev progress reporting wired up:** `./Build report` /
  `tools/objdiff-report.py` emit a valid objdiff v2 report (verified against the
  real `report.proto` with protoc); `.github/workflows/report.yml` uploads it as
  the `jp_report` artifact. Build-free (reads the committed
  `config/functions.main.exe.tsv` + config), so CI is Python-only. decomp.dev is
  artifact-driven and still needs a GitHub repo (private OK with a self-hosted
  instance) — full setup in [`docs/decomp-dev.md`](docs/decomp-dev.md).

New session: read this file + [`docs/README.md`](docs/README.md) (the docs index).

## What was actually wrong (fixed 2026-07-02)

The **build environment**, not the decomp logic:

- **Reproducibility hole (fixed).** The nix devShell shipped bare `pkgs.ghc` +
  `cabal-install`, so Shake's deps (shake/aeson/uuid/hashable) weren't in GHC's
  package DB. `./Build` ran `cabal v2-run`, needing a Hackage index + network to
  compile them into `~/.cabal`. A fresh checkout (or reset `~/.cabal`) broke the
  build with `unknown package: uuid`.
  → devShell now uses `haskellPackages.ghcWithPackages [shake aeson uuid hashable]`
  and the `Build` wrapper compiles `Build.hs` with `ghc` directly (no cabal).
  Proven to build **offline with `~/.cabal` absent**.
- **INCLUDE_ASM deps untracked (fixed).** `.c.o`/`.s.o` objects didn't depend on
  the `.include`'d nonmatching `.s` / `include/macro.inc` (only cpp `-MMD` headers
  were tracked), so editing an asm left a stale object → phantom byte-mismatch.
  → objects now assemble with `as --MD` and `need` the parsed+normalised includes.
  Verified: editing a nonmatching `.s` now re-assembles its object.
- **Asset `.bin.o` (fixed, dormant).** Rule did a raw `copyFile'`; the linker
  places assets as ELF `x.bin.o(.data)`, needing a real relocatable object.
  → now `ld -r -b binary` (matches the old Makefile). Fires once assets exist.
- **`check` hardened.** Now asserts the reference `disks/tenchu/main.exe` matches a
  pinned expected sha256, so a swapped/corrupt base image can't pass green.

## Original-source data recovered (2026-07-10)

The Japanese demo disc (PAPX-90029) shipped `PSX.SYM`, the Psy-Q linker's debug
output for an earlier, lost build. It is now parsed, dumped and mined — see
[`docs/psx-sym.md`](docs/psx-sym.md), which is the resume point for this whole thread.

- **Every game function is carved** (555/555), so `permute.py`/`m2c`/the sweep work on
  any of them. `tools/progress.py` is the live count (206/555 game functions when this
  was written).
- **Recovered from `PSX.SYM`**: 442 prototypes with the authors' parameter names, 2516
  locals (name, type, register-or-stack), 415 structs/unions/enums with real field
  names, the 31-file translation-unit map, 304 `static` declarations, 567 typed
  globals. All under `reference/psxsym-*`; `tools/matcher-prompt.py` injects the
  per-function facts into every agent launch.
- **45 function names and 156 data symbols adopted**, all byte-identical. Eleven replaced
  guesses with originals (`handle_char_state_using_item_` → `ActITEM`). 121 of the data
  symbols were already sitting unused in our own Ghidra export — `import_symbols.py`
  could only *rename*, never *define*, until now.
- **Recorded, not dropped**: `reference/psxsym-candidates.tsv` (suggestions too weak to
  adopt), `psxsym-unplaced.tsv` (46 demo functions with no retail home),
  `psxsym-unnamed.tsv` (78 retail placeholders with no candidate),
  `psxsym-data-candidates.tsv`.

Best remaining leads, roughly in value order:

1. **Match more functions.** Every batch of function renames unlocks more data symbols
   (`datamatch.py` can only see a global through a function named on both sides).
   Re-run `tools/datamatch.py` after each; it proposes zero today because it has
   harvested everything the current 997 shared names reach.
2. **`MENU.EXE` / `ENDING.EXE`.** The 61 unplaced demo functions — `OPENING.C`,
   `OPMOVIE.C`, `MOJI.C` — are presumably there; the demo was one monolithic
   `PSX.EXE`. The same four matchers apply. The build now carries all six
   executables (`./Build check-all`), each split to a single `data` blob and
   reassembled byte-identically, so a function can be carved out of any of them
   with the usual `reverse.py` workflow — see `docs/build-system.md`. The
   name-recovery matchers (`symmatch`/`xbuildnames`/`callmatch`/`datamatch`) still
   hardcode `main.exe`; they need a `--target` before they can be pointed at the
   others.

   `tools/xexe.py` already locates main.exe's functions inside the other five by
   normalized instruction identity (relocations masked). `trial.exe` -- the Mission
   Editor, a near-complete relink of the engine -- contains 891 of our 1322
   functions unambiguously, 582 of them in end-to-end runs (consecutive functions
   landing exactly back-to-back, which corroborates the boundaries), and 140 of
   them are functions we have already byte-matched. menu/ending/slps_019.01 carry
   ~600 each (engine core + the statically linked Sony SDK); run.exe only 150.

   That is a symbol-recovery path for exes that have *no* symbols at all: mint
   `config/symbols.<target>.txt` from `xexe.py --tsv`. Do it deliberately, not in
   bulk -- and note that a byte-*exact* transfer is rare (92 functions for
   trial.exe, only 4 of them ours), because the two builds place data at different
   addresses, so anything touching a global re-links differently. The C is what
   transfers, not the bytes.
3. **The `statics` list** (73 functions, 231 objects) should feed `tools/gpsyms.py`: a
   `static` never gets a `%gp` extern.
4. **The SLD stream** (per-instruction line deltas) and the `0x90`/`0x92` block markers
   are parsed and discarded. They would give an address→source-line map for the demo
   build.
5. ~~`StageBosses`/`StageEnemies` shift~~ **RESOLVED, names are correct.** Retail's
   `Think3callaid` loads `0x80097c76`/`0x80097c78` together and writes them back `+1`/`-1`
   — an adjacent pair used as a pair, exactly like the demo's `StageEnemies`/`StageCitizens`.
   Retail *inserted* a new short (`StageBosses`, a Ghidra name absent from PSX.SYM) at
   `…c74`. The datamatch vote that pointed `…c74` at `StageEnemies` was the artefact:
   `difflib` aligns the demo's store to the *first* identical retail store, which is
   exactly what an insertion looks like. **Reference-alignment votes are weakest at an
   insertion point** — corroborate with how the symbols are *used together*.

## Roadmap / TODO (not yet done)

Detailed dev docs live in [`docs/`](docs/). Ranked next steps:

0. **The `Act*` character-action family — one coherent batch. TU NOW
   ESTABLISHED.** `ActSYURI` (676 B) and `ActHANG` (700 B, jump table) are
   MATCHED (commit pending) — these proved the shared TU facts, so the remaining
   11 (`ActNORMAL`, `ActMOVE`, `ActSQUAT`, `ActACTION`, `ActITEM`, `ActATTACK`,
   `ActENGAGE`, `ActCHASE`, `ActDAMAGE`, `ActSWIM`, `ActSTICKON`, `ActSTATE`) are
   now much cheaper — Sonnet-viable for the mid-size ones, Fable only for
   `ActATTACK` (6.6 KB, the single biggest unmatched function in the game).
   Established facts:
   - **gp smalls** are MOTION.C's own: `dtM, Me_MOTION_C, motID, D_80097F0E`
     (+ `dtV, dtL, dtPAD` as used per function; `tools/gpsyms.py <Name>` after a
     build). **`StagePlayer` and `GlobalAreaMap` stay ABSOLUTE, not gp.** Add
     every sibling's Build.hs + permute.py entry together and `tools/symcheck.py`
     after each, or a converted stub silently relocates the shared data region.
   - **Structs — item.h is already complete for this family**: `MotionManager`
     (`mid`@0/`count`@2/`loop`@4), `Humanoid` (`attribute`@4, `pad.trig`@0x14,
     `model`@0x58), `ModelArchiveType.object`@0x68. PSX.SYM's `PARAM_ITEM_LAUNCH`
     == item.h's `PARAM_ITEM_USE`.
   - **Idioms** (see cookbook): jump-table dispatch on `mid` is
     `switch ((short)(ptr->mid - 0xA00))`; `D_80097F0E = 1;` is written literally
     per if/else arm (never hoisted); load-width is per-SITE not per-TU; case
     bodies lay in source order (read the order off piece memory order).
   - `ActSTICKON` and `DamageControl` additionally divide by a variable (target
     asm has `div`+`break` guards), so they need `extra "<Name>" =
     ["--expand-div"]` in Build.hs and permute.py — see the cookbook's
     `--expand-div` rule. `HumanActionControl` (the dispatcher) is drafted with
     the full list; use it plus the two matched siblings as templates.

1. **Reverse more functions.** The pipeline is now proven end-to-end on a
   gp-using function (`Think1sleep`). Use `tools/reverse.py <name> --ghidra-export
   .shake/ghidra-export` to split the next one, turn Ghidra's C in the comment into
   matching C (its return type + accumulator *types* matter — `Think1sleep` needed
   `short`/`ushort`, not `s32`), and lean on `tools/permute.py` for residual
   register allocation. `.shake/ghidra-export/c/<addr>.c` is the reference.
2. **decomp-permuter is wired in** (`tools/permute.py`) — see
   [`docs/permuter.md`](docs/permuter.md). (Note: for pure register-allocation
   ties it can stall; the faster path is often reading Ghidra's C for the right
   *variable types/count*, as with `Think1sleep`'s single `ushort` accumulator.)
3. **Commit the disassembly**: move splat's asm to a committed `asm/main.exe/` and
   set splat `base_path: .` so a fresh clone is self-contained.
   (NOTE: the operator prefers keeping `.shake/gen` regenerated-on-demand; only do
   this if that changes — [`docs/project-layout.md`](docs/project-layout.md).)
4. ~~Pin `tools/cc1-281`~~ **DONE** — nix `fetchurl` of decompals/old-gcc 0.17
   `gcc-2.8.1-psx` (sha256-pinned, `cc1-281` on PATH); the committed binary is
   gone (it was a wrong build — see [`docs/toolchain.md`](docs/toolchain.md)).
5. **CI**: a GitHub Actions job running `nix develop --command ./Build check`.
6. **Per-function tooling**: `diff_settings.py` (asm-differ is in the devShell),
   `objdiff.json`, an `m2ctx.py` context generator, a `make <obj>` shim.
7. **Growth-mod streaming safety** (if `iso-mod` movies/XA glitch): keep `main.exe`
   same-size on disc by relocating the mod region into a separate disc file rather
   than appending — see the caveat in [`docs/building-an-iso.md`](docs/building-an-iso.md).

## Progress & the SDK endgame

`tools/progress.py` (add `--json` for machine-readable/frogress-shaped output)
reports matched functions/bytes split **game code vs SDK** — the split matters
because ~1000 of the 1623 functions above `0x80061000` are statically-linked
PSY-Q library code (LIBSND/LIBSPU/LIBCD/…), not game logic. The provisional
boundary lives in the tool; refine it as library identification improves.

**Full compilation is already a permanent property**: every unmatched function
is assembled from its extracted asm (INCLUDE_ASM), so the build links a
complete, byte-identical exe at every stage — matching only ever replaces
blobs with C. The realistic goal metric is **100% of game code matched**; for
the SDK the options (in scene-standard order) are:

1. **Leave it as INCLUDE_ASM** — shippable "source + vendored asm" forever.
2. **Link original PSY-Q objects**: convert SDK `.OBJ`/`.LIB` members with
   `psyq-obj-parser` (ships with pcsx-redux, already checked out) and let the
   linker use them instead of asm blobs. Needs a user-provided PSY-Q SDK
   archive (uncommitted, like the game disc). **Split this in two** — the
   halves have very different cost/benefit:
   - *Identification* (SDK + psyq-obj-parser + `tools/coddog compare-raw`):
     names/boundaries for the ~1021 lib functions and exact per-library
     version knowledge (games shipped MIXED lib versions). High value, zero
     build risk. Do this once SDK archives are on disk.
   - *Link-swap* (actually feeding the objects to ld): requires
     reconstructing the original member link order at our fixed addresses,
     yaml/linker-script restructuring, migrating ~1000 script-assigned
     symbols to object definitions — and the built exe comes out byte-for-
     byte THE SAME as today (the bytes are already in via INCLUDE_ASM). It
     buys provenance cleanliness only; a permanent bring-your-own-SDK tax on
     contributors/CI. Defer until "no blobs" matters as a property.
3. **Opportunistic decompilation of trivial clusters** — `tools/coddog
   cluster` found e.g. 108 byte-identical `dmyGsPrstF3NL`-style stubs; one C
   file per cluster is a cheap, large jump in the SDK numbers.

Recommended: (1) now, chase the game-code metric, revisit (2) if a "no blobs"
build is ever wanted; (3) whenever a quick win is nice. Progress uploading
(frogress / decomp.dev) can consume `tools/progress.py --json` from CI later.

## Modding / non-matching builds

- **Same-size edits**: `./Build` (without `check`) produces a valid runnable
  binary; only the changed bytes differ.
- **Modified functions**: `./Build mod` — put the function in
  `src/mod/main.exe/<fn>.c` and it's compiled and **patched in place** (it must
  fit its original slot; mkmod aborts with the overage otherwise), so
  `main_mod.exe` stays the same size and the disc rebuild stays byte-faithful.
  See [`docs/modding-and-nonmatching.md`](docs/modding-and-nonmatching.md) and
  `tools/mkmod.py`.

## Running in an emulator (`./Build iso`)

`./Build iso` rebuilds the game's CD image with our `main.exe` (via mkpsxiso,
packaged in `nix/mkpsxiso.nix`) → a `.bin`/`.cue` for pcsx-redux. Matching build →
data track byte-identical to the original except `main.exe`; `./Build iso-mod`
puts the grown `main_mod.exe` on the disc (auto-LBA). Needs the original disc
(`TENCHU_CUE=…` or under `disks/`/`~/tenchu-iso/`). See
[`docs/building-an-iso.md`](docs/building-an-iso.md).

## Notes

- Generator oracle detects staleness by the *set* of generated file paths, not
  contents. Harmless now that the INCLUDE_ASM dep edge exists; do **not** "fix" it
  with hash-retrigger — that would revert hand-edits by re-running splat.
- Reproducibility depended on a warm `~/.cabal` before the nix fix; it no longer does.
