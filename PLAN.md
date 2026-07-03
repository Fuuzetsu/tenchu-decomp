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
- **Matched functions:** `initialise_font`, `GetRealPad` (via decomp-permuter —
  [`docs/matching-get-held-buttons.md`](docs/matching-get-held-buttons.md)),
  **`Think1sleep`** — the first function needing `$gp`-relative globals — and
  **`ProcItemManebue`** — the first needing *absolute* addressing of a small
  global that another TU gp-addresses — and **`ProcItemKusuri`** (1432 bytes,
  the largest yet: switch dispatch, casted scratch buffer, magic divisions,
  block-copy loop, cross-jumped tails), and **`ReqItemDrop`** — the first
  cross-referenced match: called by the compiled `ProcItemKusuri` (real
  R_MIPS_26 link), sharing types via `src/main.exe/item.h`.
  [`docs/matching-cookbook.md`](docs/matching-cookbook.md) records the idioms. Between them they pinned down the real
  gp model (ASPSX gp-addresses only TU-local definitions; externs are absolute)
  and produced the reusable infrastructure in
  [`docs/toolchain.md`](docs/toolchain.md): the opt-in `maspsx --gp-extern`
  patch + per-file lists in `Build.hs` (`maspsxGpExterns`), and the `macro.inc`
  `li→addiu` fix. The compiler is the canonical decompals `gcc-2.8.1-psx`,
  nix-pinned and verified equivalent to real Sony `CC1PSX.EXE` (decomp.me
  `psyq4.3`) — see `compiler-fidelity` in the toolchain doc.
- **Modding + emulator loop works:** `./Build mod` (grow functions) and
  `./Build iso`/`iso-mod` (bootable disc for pcsx-redux). See docs/.

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

## Roadmap / TODO (not yet done)

Detailed dev docs live in [`docs/`](docs/). Ranked next steps:

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
   archive (uncommitted, like the game disc). Also unlocks precise SDK
   function identification/naming via `tools/coddog compare-raw`.
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
