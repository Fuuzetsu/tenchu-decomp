# Tenchu (PS1) decompilation — developer docs

This directory records how the build works, what state it's in, and the
decisions behind the toolchain. It's the reference companion to the terse
[`PLAN.md`](../PLAN.md) at the repo root.

## Contents

- [build-system.md](build-system.md) — the split→reassemble pipeline, the Shake
  build driver, dependency tracking, and the offline/reproducible nix setup.
- [toolchain.md](toolchain.md) — the compiler + assembler story, **maspsx vs
  ASPSX.EXE** (do we need wine?), the ASPSX version for Tenchu, and the exact
  recipe to add maspsx.
- [project-layout.md](project-layout.md) — recommended directory layout drawn
  from established PSX decomps, what to keep vs change, and Shake-vs-Make.
- [matching-get-held-buttons.md](matching-get-held-buttons.md) — a worked
  case study of trying to byte-match a real function, and what makes it hard.
- [modding-and-nonmatching.md](modding-and-nonmatching.md) — building
  non-matching binaries (behaviour changes, debug): what works (same-size edits)
  and how to handle growth.
- [building-an-iso.md](building-an-iso.md) — `./Build iso` / `iso-mod`: rebuild a
  bootable CD image (`.bin`/`.cue`) for pcsx-redux with our `main.exe`.
- [permuter.md](permuter.md) — `tools/permute.py`: run decomp-permuter to
  byte-match register-allocation-hard functions.
- [ghidra-bridge.md](ghidra-bridge.md) — pull decompiled C from your Ghidra
  project into `src/` one function at a time (`tools/reverse.py`).

## Current state (verified 2026-07-02)

The disassemble → reassemble round-trip **works and is byte-identical**:

```console
$ nix develop            # or: direnv allow, once
$ ./Build check          # clean-rebuilds and asserts sha256 == the real main.exe
BUILD GREEN (byte-identical)
```

- `disks/tenchu/main.exe` (the real game exe) is split by **splat**
  (`split.py`) into per-function ASM, data, a header, C stubs, and a linker
  script; everything is reassembled with the GCC 2.8.1 cross toolchain + GNU
  `ld` back into a **byte-identical** `main.exe`
  (sha256 `0690a5c1…3558`).
- `initialise_font` and `get_held_buttons` are **fully decompiled C functions
  that byte-match** — proof the pipeline handles real C, not just `INCLUDE_ASM`
  stubs. `get_held_buttons` was matched with the **decomp-permuter** (see the
  case study). `think_setting_sleep` is still a WIP stub.

## What was wrong, and what got fixed

The decomp logic was fine; the **build environment** was broken and had a few
latent bugs. Fixed in this batch of work (see `build-system.md` for detail):

1. **Reproducibility** — the nix devShell shipped bare `ghc`+`cabal`, so
   `./Build` needed network + a warm `~/.cabal` to compile Shake's deps. Now the
   deps are baked into GHC via `ghcWithPackages` and `./Build` compiles the
   driver with plain `ghc` — fully offline on a fresh checkout.
2. **INCLUDE_ASM dependency tracking** — objects didn't depend on the `.include`'d
   nonmatching `.s` / `macro.inc`, so asm edits produced silently stale output.
   Now tracked via `as --MD`.
3. **Asset `.bin.o`** now uses `ld -r -b binary` (a raw copy would fail the link).
4. **`check`** now pins the expected sha256 to catch a swapped/corrupt base image.

## Also built this cycle

- **maspsx integrated** — `cpp | cc1-281 -G8 | maspsx | as`, so functions using
  integer division / `$gp`-relative globals can byte-match. wine dropped. See
  [toolchain.md](toolchain.md).
- **`./Build mod`** — grow/modify functions runnably via trampolines + a mod
  region. See [modding-and-nonmatching.md](modding-and-nonmatching.md).
- **`./Build iso` / `iso-mod`** — rebuild a bootable `.bin`/`.cue` for pcsx-redux.
  See [building-an-iso.md](building-an-iso.md).

## Next step

Finish `think_setting_sleep` (the active matching target) — see `PLAN.md` #1 and
the analysis in `src/main.exe/think_setting_sleep.c`: gp globals as tentative
definitions, `character_state` type-size fixes, then a permuter pass.
