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
- **Matched functions:** `initialise_font` and `get_held_buttons` (the latter via
  decomp-permuter — [`docs/matching-get-held-buttons.md`](docs/matching-get-held-buttons.md)).
  `think_setting_sleep` is a WIP stub (see roadmap #1).
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

1. **Finish `think_setting_sleep`** (the active matching target). maspsx made it
   very close; the exact remaining work is documented in the stub
   (`src/main.exe/think_setting_sleep.c`) and [`docs/toolchain.md`](docs/toolchain.md):
   (a) declare its gp globals as tentative definitions (drop `extern`) so maspsx
   emits `%gp_rel` — the *reusable* gp pattern; (b) fix `character_state` type
   sizes (`character_kind`/`character_status` → 2 bytes, `button_mask` → 2 bytes so
   `some_character_button_values` is 16) and `frames_since_animation_start` → `s16`;
   (c) a decomp-permuter pass for any residual register allocation.
2. **Wire decomp-permuter into the repo** (`compile.sh` + `import.py`) — the manual
   setup that matched `get_held_buttons` is in the memory note / case study; make
   it a first-class `tools/` thing for reuse.
3. **Commit the disassembly**: move splat's asm to a committed `asm/main.exe/` and
   set splat `base_path: .` so a fresh clone is self-contained.
   (NOTE: the operator prefers keeping `.shake/gen` regenerated-on-demand; only do
   this if that changes — [`docs/project-layout.md`](docs/project-layout.md).)
4. **Pin `tools/cc1-281`** via a checksummed nix `fetchurl`
   (decompals/old-gcc `gcc-2.8.1-psx`) instead of an opaque committed binary.
5. **CI**: a GitHub Actions job running `nix develop --command ./Build check`.
6. **Per-function tooling**: `diff_settings.py` (asm-differ is in the devShell),
   `objdiff.json`, an `m2ctx.py` context generator, a `make <obj>` shim.
7. **Growth-mod streaming safety** (if `iso-mod` movies/XA glitch): keep `main.exe`
   same-size on disc by relocating the mod region into a separate disc file rather
   than appending — see the caveat in [`docs/building-an-iso.md`](docs/building-an-iso.md).

## Modding / non-matching builds

- **Same-size edits**: `./Build` (without `check`) produces a valid runnable
  binary; only the changed bytes differ.
- **Growth (debug prints, bigger functions)**: `./Build mod` — put the grown
  function in `src/mod/main.exe/<fn>.c` and it's relocated into a mod region
  (`MODBASE=0x80098000`) with a trampoline at the original slot, so the rest of
  the binary stays byte-identical. See
  [`docs/modding-and-nonmatching.md`](docs/modding-and-nonmatching.md) and
  `tools/mkmod.py`. (Runtime-safe `MODBASE` may need the game's memory map.)

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
