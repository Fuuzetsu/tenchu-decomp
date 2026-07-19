# Non-matching builds: modding & debug

You don't always want a byte-match — modifying behaviour, adding debug output,
cheats, etc. This documents what works today and how to make deviations run.

## Choose the build contract

The default and relink commands have deliberately different jobs:

```console
$ ./Build             # reference layout/matching branches; same-size edits only
$ ./Build mod         # patch selected same-slot sources into main_mod.exe
$ ./Build relink      # normal GNU-ld layout; functions/data/BSS may move
$ ./Build check-relink # structural audit plus full +0x10004 growth proof
```

`./Build check` is only for verifying that the reference decomp still matches
retail. It is not the size-changing mod gate. For a normal source edit, build
`main_relink.exe` and launch it with `run-relink` or package it with
`iso-relink`.

## What works: same-size edits (verified)

An edit that keeps the function the **same size** (same instruction count)
produces a valid, runnable binary. Verified: changing `arg0 >> 4` to `arg0 >> 5`
in `get_held_buttons` yields a binary that is **byte-identical except the one
changed instruction** — same total size, PS-EXE header untouched. Everything
else stays at its original address, so all the game's pointers (symbolic and raw)
are still correct.

Same-size covers a lot: changing constants/immediates, swapping a branch
condition, changing which register/field is used, reordering independent ops,
tweaking arithmetic. Prefer these when you want a drop-in behaviour change.

## Why size changes break the reference lane

Adding or removing instructions (e.g. a `printf` debug line, an extra call)
**changes the function's size**. Verified: adding a second call to
`get_held_buttons` grew the binary by 8 bytes and made ~68% of the file differ —
because everything laid out **after** the edited function shifts down by 8.

The reference build may still link, but that artifact is **not the right
runtime contract** for a size change:

- The linker script places sections **sequentially** from `0x80011000`, so a
  function that grows pushes every later function and data block down.
- The reference data is a mix of symbolic directives and retail raw bytes, and
  its symbol scripts intentionally retain fixed retail assignments.
- When code shifts, the symbolic references move with it but the raw/fixed ones
  don't. The two disagree → the binary is internally inconsistent → it crashes.
- The PS-EXE header's `.text` size (`0x00087000`, hardcoded in the generated
  `header.s`) also no longer matches the grown image.

In short: the exact reference lane can absorb *substitutions* but is not the
normal-link lane for *insertions*.

## Existing convenience lane: `./Build mod` — patch in place

Non-matching behaviour changes (a decompiled function edited however you like) are
handled by **overwriting the function in place**, wired in as the `mod` target:

```console
$ # put your modified function in src/mod/main.exe/<fn>.c (filename == fn)
$ ./Build mod        # -> .shake/build/tenchu/main_mod.exe
mkmod: hooking ProcItemKusuri
  ProcItemKusuri: 1376/1432 bytes -> patched in place @ 0x80040500
mkmod: wrote .shake/build/tenchu/main_mod.exe (555008 bytes, same size as main.exe …)
```

How it works (`tools/mkmod.py`): it starts from the byte-matching `main.exe`, then
for each `src/mod/main.exe/<fn>.c`:

1. compiles it with the normal pipeline (`cpp | cc1 -G8 | maspsx | as`);
2. links it at the function's **original address**, resolving the game's other
   symbols from `main.exe.elf`;
3. overwrites that function's bytes in `main.exe` with the result.

The function must **fit in its original slot** (its address → the next symbol). Since
nothing moves, `main_mod.exe` is byte-identical to `main.exe` except that function —
same size, PS-EXE header untouched — so the disc rebuild keeps forced LBAs and it runs
on any emulator or real hardware. `src/main.exe/` (the matching decomp) is untouched;
mods live only in `src/mod/main.exe/`.

This target is intentionally in-place. It remains useful for a minimal
byte-faithful disc diff, but it is not the architecture for size-changing edits and it does
not use trampolines or a hidden mod region. A genuine relink can move code,
loaded data, BSS, and linker-derived boundaries within the executable's RAM
budget.

**If it doesn't fit**, `./Build mod` aborts with the overage, e.g.:

```
mkmod: ProcItemKusuri compiled to 1440 bytes but its slot (…) is only 1432 — over by 8.
Trim it (drop debug logging / simplify) so it fits in place …
```

Trim the function — drop debug/log calls, simplify Ghidra-isms — until it fits. (The
worked example `ProcItemKusuri.c` drops the original's `AdtMessageBox("item dispose
fail…")` diagnostic, which is never hit in normal play, to make room.)

## Size-changing normal relink

The static normal-link path is implemented. Edit a game source normally, add a
helper under `src/main.exe/reloc/` when useful, then run:

```console
$ ./Build check-relink
$ ./Build run-relink       # fast direct MAIN.EXE launch
$ ./Build iso-relink       # package the actual normal-link executable
$ ./Build run-iso-relink   # full SLPS/MENU/MAIN boot
```

The normal lane retains the pinned compiler's ordinary output sections for
both existing game objects and new helper files. That includes small and large
initialized/tentative globals, statics, const/float/double data,
`.sdata`/`.sbss`/`.scommon`, `.bss.*`, and GNU `COMMON`; gp-small objects are
kept within signed GPREL16 reach. “Whatever C” is scoped to those normal
toolchain sections and available PS1 RAM, not arbitrary custom section
attributes or unlimited data.

The composed linker owns all 555 game inputs, the complete CRT/SDK stream,
initialized data, `_gp`, BSS, allocator boundaries, and the PS-X header. The
reviewed data transform supplies 208 `R_MIPS_32` pointer records. The widened
audit reports zero movable absolute symbols, literal jumps, adjacent address
pairs, or data pointers.

`check-relink` also performs a real GNU-ld link with `0x10004` bytes inserted
after `main.c.o`. That proof checks 7,706 owned symbols, all 208 loaded pointers,
HI16 carry behavior, BSS movement, dynamic allocator shrinkage, and a changed
PS-X entry/load size. The linker chose PC `0x80070260` and `t_size=0x97000` for
the grown fixture; no per-function slot, cave, trampoline, or fixed downstream
address is involved.

The current controlled growth artifact also passes the bounded PCSX-Redux
runtime probe in both modes: direct `-loadexe` and an auto-LBA repack through
`SLPS_019.01 → MENU.EXE → MAIN.EXE`. The harness reached entry `0x80070260`,
ELF-derived `main` at `0x800162a4`, moved `PadProc` at `0x8002adac`, and later
VSyncs without a first-chance exception. This establishes the grown boot/main
loop, not every gameplay path or arbitrary edit. Growth remains bounded by the
PS1 RAM map, the fixed cross-executable handoff, allocator/stack space, and
MIPS relocation ranges. Representative STR and XA playback remain runtime
validation gates. See
[relocatable-build.md](relocatable-build.md) and
[the exact smoke commands](building-an-iso.md#automated-grown-image-smoke).

## Running a modified exe

Both mod launchers use the same-size `main_mod.exe`, so both are byte-faithful except
your function (see [building-an-iso.md](building-an-iso.md)):

- `./Build run-mod` — `-loadexe main_mod.exe` over the original disc. Fast, but boots
  `MAIN.EXE` directly (skips `SLPS→MENU`), so some game state is under-initialised.
- `./Build run-iso-mod` — **faithful**: repacks the disc with `main_mod.exe` (forced
  LBAs, streamed cutscenes intact) and runs the full `SLPS→MENU→MAIN` boot. Use this
  to actually play a mod.

For a size-changing normal link, use `run-relink` for the fast direct launch and
`run-iso-relink` for the full launcher/media path. Those commands consume
`main_relink.exe`, not the fixed-slot mod artifact. For a bounded automated
check, point `tools/pcsx_smoke.py` at that executable and its ELF; add
`--repack` to exercise the launcher/menu path. A real mod should still exercise
its changed behavior and relevant STR/XA content rather than treating the boot
smoke as comprehensive coverage.
