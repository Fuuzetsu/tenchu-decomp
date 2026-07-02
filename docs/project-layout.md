# Project layout — recommendations

The current layout was assembled ad-hoc. It works, but a couple of choices make
the repo **not self-contained**. Below is a target layout drawn from established
splat-based PSX decomps (SOTN, `esa`, Silent Hill), with what to keep vs change.

## The one important bug: committed asm

Today `src/main.exe/*.c` do
`INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/…", …)`, but
`.shake/` is git-ignored. **A fresh clone cannot assemble the tree** — the
disassembly it depends on only exists after re-running splat locally. Established
projects **commit** the disassembly and generated stubs as the source of truth,
and only git-ignore the build outputs.

Fix: move splat's asm output under a committed `asm/main.exe/` and set
`base_path: .` so the include collapses to the clean
`INCLUDE_ASM("main.exe/nonmatchings/Think1sleep", Think1sleep)`
form (matching SOTN/esa). The `include/include_asm.h` macro already emits
`.include "FOLDER/NAME.s"`, so only the FOLDER string and the committed location
change.

## Target tree

```
config/                       flat, one yaml + symbols per binary  (KEEP)
  splat.main.exe.yaml         CHANGE base_path: . ; asm_path: asm/main.exe ; src_path: src/main.exe
  splat.slps_019.01.yaml      per-overlay (add others as tackled)
  symbols.common.txt          NEW: PSX hw/BIOS regs split out of symbols.main.exe.txt
  symbols.main.exe.txt        game symbols only
  main.exe.sha256             NEW: externalise the expected hash out of Build.hs
asm/main.exe/                 NEW, COMMITTED (today only in .shake/gen — the bug)
  nonmatchings/<cfile>/<func>.s
  data/*.s   header.s
src/main.exe/                 KEEP (committed); write matched C here, overrides stubs
  *.c   main.exe.h
assets/main.exe/              extracted bins (decide: commit vs documented re-extract)
include/                      KEEP: common.h, include_asm.h, types.h, macro.inc, psxsdk/*
tools/                        maspsx (added) + pinned cc1  (see toolchain.md)
.shake/                       KEEP but build-only: Shake DB, *.o, linked exe, .map,
                              generated .ld, undefined_*_auto — never source of truth
```

**Keep as-is:** the flat per-binary `config/`; the `ld -r -b binary` asset rule;
splat `section_order` already listing `.sdata/.sbss/.bss`; the nix-overlay
approach for tools (more reproducible than git submodules).

**Nit:** `diff_settings.py` points at `.shake/build/tenchu/main.exe` (linked exe)
while splat `build_path` is `.shake/build/main.exe` (object dir). They're
different dirs by design; just name them consistently to avoid confusion.

## Keep Shake — don't rewrite to Make

The "a decomp must use Make" belief is a myth: the two most-cited PSX decomps use
non-Make drivers (SOTN = CMake, Silent Hill = Ninja) with a thin Makefile wrapper
only for the `make <obj>` convention. Shake earns its keep here — `Build.hs`
already solves the correctness-critical INCLUDE_ASM dependency tracking (a naive
Makefile gets this wrong) and just gained offline nix reproducibility. Rewriting
would re-introduce a solved bug for no real gain.

The genuine ecosystem need — tools default to `make <target>`, and the single-
object rebuild is the core matching-loop operation — is met by a small shim, not
a rewrite:

1. A `Makefile` whose `%.c.o`/`%.s.o`/`%.o` rules just `exec ./Build $@` (Shake
   already accepts a single `.o` path as a target).
2. `objdiff.json` with `custom_make: ["make"]`, `watch_patterns: ["*.c","*.s"]`,
   units pointing at `.shake/build/main.exe/<file>.c.o` and expected objects
   under `expected/`.
3. `diff_settings.py` `make_command` so `asm-differ`'s `diff.py` rebuilds via the
   same entry point.

`decomp.me` / `m2c` / `decomp-permuter` never invoke the project build (they work
on a single object + a context header), so Shake-vs-Make is irrelevant to them —
what they need is the exact published flag set and an `m2ctx.py` context
generator. Revisit a Make migration only if contributor onboarding friction
(nobody knows Shake/GHC) ever outweighs Shake's dependency correctness, and only
after maspsx + committed-asm land.
