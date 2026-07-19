# Tenchu (PS1) decompilation — developer docs

This directory records how the build works, what state it's in, and the
decisions behind the toolchain. It's the reference companion to the terse
[`PLAN.md`](../PLAN.md) at the repo root.

## Contents

- [build-system.md](build-system.md) — the split→reassemble pipeline, the Shake
  build driver, dependency tracking, all six executables, and the
  offline/reproducible nix setup.
- [toolchain.md](toolchain.md) — the compiler + assembler story, **maspsx vs
  ASPSX.EXE** (do we need wine?), the ASPSX version for Tenchu, and the exact
  recipe to add maspsx.
- [project-layout.md](project-layout.md) — recommended directory layout drawn
  from established PSX decomps, what to keep vs change, and Shake-vs-Make.
- [orchestration.md](orchestration.md) — **the runbook for driving matcher
  agents at scale**: the tool map, how to launch/prompt/harvest agents, model
  routing, the bundling economics (bundle by FAMILY), the reflection loop, and
  the high-leverage backlog. Read this to resume the batch-matching work.
- [flywheel-handoff.md](flywheel-handoff.md) — the latest dated clean-shutdown
  anchor: resume preflight, parked residuals to avoid repeating, target
  priorities, and agent/worktree hygiene.
- [matching-cookbook.md](matching-cookbook.md) — **start here for matching
  work**: the cc1 2.8.1 source idioms that byte-match (dispatch, loops, fold
  reassociation, stack buffers, regalloc steering), the `tools/matchdiff.py`
  iteration loop, and pointers to the worked examples.
- [gte-policy.md](gte-policy.md) — the restricted GTE inline-asm policy
  (adopted 2026-07-16): why COP2/GTE has no C spelling, the `src/main.exe/gte.h`
  macro layer, the whitelist (`config/gte-allowlist.txt`), enforcement, and the
  family matching order.
- [psyq-headers.md](psyq-headers.md) — the clean minimal PsyQ ABI shims, their
  comparison against real 4.5 headers, and why the default build does not fetch
  or vendor Sony's SDK.
- [relocatable-build.md](relocatable-build.md) — why shiftability does not
  require decompiling every PsyQ routine into C, how MGS/Silent Hill/SOTN handle
  stock SDK code, the exact `GS_107.OBJ` relocation proof, the implemented
  linker-owned game/SDK-prefix/BSS gates, and the remaining path to a grown
  executable.
- [relocatable-data.md](relocatable-data.md) — the loaded-data pointer
  inventory, exact-interior-label policy, and the first manifest-driven
  `R_MIPS_32` slice with `+4`/`+0x10004` linker proofs.
- [psx-exe-finalizer.md](psx-exe-finalizer.md) — the PS-X EXE
  finalizer/validator used by the normal-link BSS proof: ELF/map symbol
  resolution, sector padding, regenerated PC/load/size fields, and header
  invariants.
- [psyq-object-lane.md](psyq-object-lane.md) — run the implemented, opt-in
  `GS_107.OBJ` link audit against a user-supplied PsyQ 4.4–4.6 `LIBGS.LIB`.
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
- [psx-sym.md](psx-sym.md) — the demo disc's `PSX.SYM`: the original source file
  names, prototypes with parameter names, struct/enum layouts and TU map. How we
  extract it (`tools/extract-demo.py`), parse it (`tools/psxsym.py`), dump it
  (`tools/symdump.py`) and recover retail names from it (`tools/symmatch.py`,
  `tools/xbuildnames.py`, `tools/callmatch.py` — always `--verify`).
- [decomp-dev.md](decomp-dev.md) — progress reporting on **decomp.dev**: how it
  ingests (artifact-driven objdiff reports, never `decomp.yaml`), our build-free
  generator (`tools/objdiff-report.py` / `./Build report`) + `jp_report` CI
  workflow, and standing up a self-hosted instance against a (private) repo.

## Current state (verified 2026-07-19)

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
- 537 of 555 game-code functions are **fully decompiled C functions that
  byte-match**, covering 296872 of 303244 game-code bytes (97.90%). The other
  18 are canonical handwritten-assembly originals, so game code is 555/555
  done. Run `tools/progress.py` for the authoritative current count; parallel
  SDK work makes totals stale quickly.

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
- **`./Build mod`** — compile selected mod sources and patch them into their
  original slots. This is a same-size convenience lane, not the size-changing
  normal relink. See
  [modding-and-nonmatching.md](modding-and-nonmatching.md).
- **`./Build iso` / `iso-mod`** — rebuild a bootable `.bin`/`.cue` for pcsx-redux.
  See [building-an-iso.md](building-an-iso.md).

## Next step

Resume with the preflight and live-target selection in
[`flywheel-handoff.md`](flywheel-handoff.md). Do not select work from this
README's dated progress count or an old `NON_MATCHING` comment without checking
the current source and `tools/triage.py` first.
