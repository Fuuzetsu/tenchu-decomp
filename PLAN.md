# Tenchu (PS1) decompilation — build plan

## Approach

Split the original `main.exe` into sections and per-function ASM with `splat`
(`split.py`), then progressively replace `INCLUDE_ASM` stubs with C that
recompiles to the same bytes. The build (Haskell **Shake**, `shake/src/Build.hs`,
which superseded the old `Makefile`) reassembles everything and gates on a
byte-identical `main.exe`.

## Status: the round-trip works ✅

`./Build clean && ./Build check` disassembles and reassembles to a **byte-identical**
`main.exe` (sha256 `0690a5c1…3558`, matches `disks/tenchu/main.exe`). The splitting
and reassembly are sound — verified from a clean state.

`initialise_font` and `get_held_buttons` are fully matched C (the latter via
decomp-permuter — see [`docs/matching-get-held-buttons.md`](docs/matching-get-held-buttons.md));
`think_setting_sleep` is a stub with a WIP C attempt in a comment.

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

Detailed developer docs now live in [`docs/`](docs/) — build system, the maspsx
decision + recipe, project layout, and a matching case study. Ranked next steps:

1. **maspsx** between `cc1` and GNU `as` — the one missing piece before functions
   using integer division or `$gp`-relative globals byte-match. Confirmed: it
   *replaces* ASPSX.EXE, so **wine can be deleted**. Start `--aspsx-version=2.77`,
   `-G8`; add it as a filter in the `.s` stage. Full recipe + rationale in
   [`docs/toolchain.md`](docs/toolchain.md). (Not needed for `get_held_buttons`.)
2. **Commit the disassembly**: move splat's asm to a committed `asm/main.exe/` and
   set splat `base_path: .` so a fresh clone is self-contained. See
   [`docs/project-layout.md`](docs/project-layout.md).
3. **Pin `tools/cc1-281`** via a checksummed nix `fetchurl`
   (decompals/old-gcc `gcc-2.8.1-psx`) instead of an opaque committed binary.
4. **CI**: a GitHub Actions job running `nix develop --command ./Build check`.
5. **Per-function tooling**: `diff_settings.py` (asm-differ is in the devShell),
   `objdiff.json`, an `m2ctx.py` context generator, and a `make <obj>` shim.
6. Continue matching functions: `think_setting_sleep` next (needs maspsx; C
   draft already sketched). The **decomp-permuter** workflow is established —
   `get_held_buttons` was matched with it (see
   [`docs/matching-get-held-buttons.md`](docs/matching-get-held-buttons.md)); wire
   it into the repo (`compile.sh`/`import.py`) for reuse.

## Modding / non-matching builds

`./Build` (without `check`) already builds non-matching C. Same-size behaviour
edits produce a valid, runnable binary (only the changed bytes differ). Size-
changing edits (e.g. debug prints) shift everything after the function and break
the fixed partial-decomp layout — see
[`docs/modding-and-nonmatching.md`](docs/modding-and-nonmatching.md) for the
trampoline/mod-region approach to allow growth (not yet wired in).

## Notes

- Generator oracle detects staleness by the *set* of generated file paths, not
  contents. Harmless now that the INCLUDE_ASM dep edge exists; do **not** "fix" it
  with hash-retrigger — that would revert hand-edits by re-running splat.
- Reproducibility depended on a warm `~/.cabal` before the nix fix; it no longer does.
