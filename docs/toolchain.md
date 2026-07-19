# Toolchain: maspsx vs ASPSX.EXE, and how to reach byte-matches

> **Status: maspsx is integrated.** The pipeline is `cpp | object-profile cc1
> -G8 | maspsx --aspsx-version=2.77 -G8 | as -G0 | ld`: GCC 2.8.1 is the
> default, and the reused ADT object selects GCC 2.8.0. wine has been removed
> from the devShell. `./Build check` stays byte-identical. The "Recipe" section
> below is kept as the rationale/record.

## TL;DR — do we need maspsx or ASPSX.EXE / wine?

**Use maspsx. Delete wine.** (Done.)

maspsx *replaces* `ASPSX.EXE` (+ `psyq-obj-parser`); it is not a complement. This
repo already committed to the modern, wine-free "path B" that every active PSX
decomp uses (SOTN, Silent Hill, MediEvil, Soul Reaver, Croc, Spyro):

| PSY-Q tool (path A, needs wine/DOS) | This repo (path B, native Linux) |
|---|---|
| `CC1PSX.EXE` (compiler)             | `cc1-281` by default; `cc1-280` for the reused ADT object (both nix-pinned) |
| `ASPSX.EXE` + `psyq-obj-parser`     | **maspsx** + `mipsel-…-as`  ← missing piece |
| `PSYLINK`                           | `mipsel-…-ld` + `main.exe.ld` |

So the `pkgs.wine` in `flake.nix` and the commented `CC := wine …/CC1PSX.EXE`
line in the old `Makefile` are **dead** — nothing in the pipeline needs wine or
DOS emulation. Path B is deterministic, hermetic (pinnable by hash), fast, CI-
friendly, and emits native ELF that `asm-differ`/`objdiff`/`splat` consume
directly.

Source: the [maspsx README](https://github.com/mkst/maspsx) ("replacement of the
combination of ASPSX.EXE + psyq-obj-parser"; "Native, vanilla, gcc versions make
dosemu2 and wine unnecessary").

## Why maspsx is mandatory (measured against Tenchu's binary)

GNU `as` with `.set reorder` expands MIPS pseudo-instructions and inserts
load/branch-delay `nop`s **differently** from ASPSX, and lays out `$gp`/`.sdata`/
`.comm` differently. maspsx post-processes `cc1`'s asm so GNU `as` reproduces
ASPSX's bytes. Concretely it normalises:

- `div`/`divu`/`rem`/`remu` → ASPSX's `break`-based divide-by-zero + signed-
  overflow trap sequence (`--expand-div`). *Tenchu's `main.exe` contains 172×
  `break 0x7` and 162× `break 0x6` — it uses these traps.*
- load-delay / pipeline `nop` insertion, and the `mflo`/`mfhi`/`mult`/`div` gap.
- `$at` expansion of `li`/`move` and of `lb/lbu/lh/lhu/lw`.
- `li` expansion (`li 1` → `ori 1`, etc).
- `%hi`/`%lo` macros.
- `$gp` small-data addressing (`symbol+offset` and `la` via `$gp`). *Tenchu uses
  `%gp_rel(SYMBOL)($gp)` pervasively (gp = `0x80097698`).*
- `.comm`/`.lcomm` placement.

Each is gated by a per-ASPSX-version config table in maspsx.

Because maspsx sits only between `cc1` and `as`, it does **not** touch splat, the
linker script, or `ld`, so adding it cannot regress the existing green round-trip
(the two non-decompiled functions are still `INCLUDE_ASM` .s). The gating risk is
the `-G0`→`-G8` flag change (below), which must be verified against the already-
matched `initialise_font`.

## Which ASPSX version for Tenchu?

**Start with `--aspsx-version=2.77`** (the default SOTN and Silent Hill use for
their main exes), then confirm empirically.

Evidence: all five Tenchu exes carry `Library Programs (c) 1993-1997 Sony
Computer Entertainment Inc.` (a late-1997 PSY-Q ~4.x SDK, matching the Jan/Feb
1998 JP release). The decisive signal is codegen: `main.exe` uses gp-relative
`symbol+offset` addressing, which maspsx only emulates at `--aspsx-version ≥ 2.70`
(and the README's "Known Differences" table first shows that column green at
2.77). This rules out anything `< 2.70`.

Confirm/escalate by matching **one** real gp-using function with `asm-differ`
(`diff.py`): if the `li`/`move`/div-trap/`$gp` bytes line up at 2.77, done. Only
if a `la`-via-`$gp` case mismatches, retry `2.79 → 2.81 → 2.86` (those thresholds
enable `gp_allow_la`). We have **not** seen `la`-via-`$gp` in the disassembly yet,
so don't jump there pre-emptively. Overlays (`slps_019.01`, etc.) may use a
different version — check each when tackled (SOTN, for instance, uses 2.67 for
one overlay, 2.77 elsewhere).

## Compiler versions are original-object profiles

GCC 2.8.1 remains the game's default compiler, but the contiguous reused ADT
library object at `0x8005f7f4..0x800601d4` was built with GCC 2.8.0. The build
therefore pins both `cc1-281` and `cc1-280` and selects the executable through
the same per-original-object profile oracle that owns exceptional compiler
flags. Our one-function C files are an artificial decompilation split; the
profile enumerates every carved member of the object, not a function chosen to
improve a local score:

`AdtGetDisp`, `AdtMessageBox`, `AdtQuiet`, `AdtFntOpen`, `AdtFntLoad`,
`AdtReleaseDisp`, `AdtDmyPadRead`, `AdtVsprintf`, `FUN_8005fe38`,
`FUN_8005fe88`, and `AdtSelect`.

This attribution is byte- and compiler-proven. Recompiling all eleven with
2.8.0 preserves every already-exact member and makes the clean 776-byte
`AdtSelect` exact; substituting those objects into a fresh full link gives zero
differing bytes in `main.exe`. Canonical 2.8.1 and Sony's SN32 2.8.1 build both
leave the same nine-byte residual. The responsible source change is in
`reload.c`: 2.8.0 preserves INPADDR/OUTADDR reloads as
`RELOAD_FOR_OPADDR_ADDR`, whose separate address-use bookkeeping permits the
huge-frame `lw a3,0(a3)` self-tie. 2.8.1 unconditionally retypes that case to
`RELOAD_FOR_OPERAND_ADDRESS`, whose shared conflict bit forces a second reload
register. This supersedes the earlier conclusion that the human indexed loop
was unreachable.

## Recipe: add maspsx to the pipeline (done — kept as the record)

This is how maspsx and the compiler profiles were integrated (all steps below
are done). Gating check: `./Build check` stayed byte-identical, and
`get_held_buttons`/`initialise_font` were verified unchanged under `-G8` +
maspsx *before* the flag flip.

1. **Vendor maspsx** into the devShell (it's a single-file, stdlib-only Python
   script). Mirror the `asm-differ` pattern in `flake.nix`: add a
   `fetchFromGitHub mkst/maspsx` (or a Fuuzetsu-style overlay if one exists) and
   a `writeShellScriptBin "maspsx"` that runs `python3 maspsx.py`. In the same
   edit, **delete `pkgs.wine`**.
2. **Pin `cc1-281`** (DONE). `flake.nix` `fetchurl`s
   [decompals/old-gcc](https://github.com/decompals/old-gcc) **0.17**
   `gcc-2.8.1-psx.tar.gz` (sha256 `f6f6e883…1be0`) and puts `cc1-281` on PATH; the
   committed binary is gone. **This is the *canonical* build** — verified byte-for-byte
   equal to the real Sony `CC1PSX.EXE` (== decomp.me `psyq4.3`) on our corpus + on a
   fingerprint function. The **previously-committed `tools/cc1-281` was a different,
   non-canonical build** with a codegen bug: it read the *high* halfword for
   `(short)(int)` truncation (`lhu 0x10`) where Sony reads the *low* half (`lhu 0x8`).
   Switching to canonical fixed it, kept Think1sleep/initialise_font matching, and only
   needed GetRealPad re-matched (its cleaner pointer-temp form — the old `do/while(0)`
   was an artifact of the buggy binary). Verdict: use canonical 2.8.1 by default;
   the reused ADT object is the measured 2.8.0 exception documented above. The
   real Sony 2.8.1 binary is unnecessary and 2.7.2 is worse (register alloc).
3. **Insert maspsx as a filter** (not `--run-assembler`) in the `.s`-producing
   stage of `Build.hs`. Change `cc1 ccFlags` (stdin `.c` → stdout `.s`) to pipe
   `cc1` through `maspsx --aspsx-version=2.77 -G8` (add `--expand-div` for TUs
   that do integer `/` or `%`). Leave the separate `*.c.o` rule
   (`as … --MD depFile` + `neededAsmDeps`) **unchanged** so the INCLUDE_ASM
   dependency edges keep working. Do **not** use `--run-assembler` here — that
   would move assembly into maspsx and lose the `--MD` edge.
4. **Switch codegen to `-G8`.** In `ccFlags` change `-G0` → `-G8` (keep
   `asFlags -G0`; maspsx/`as` handle gp). Pass `-G8` to maspsx too (its
   `sdata_limit`). The rest of `ccFlags` is already the standard PSY-Q/maspsx set.
5. **Gate:** `./Build clean && ./Build check` must stay byte-identical. If
   `initialise_font` regresses under `-G8`, that reveals a `.sdata`/`.sbss`
   layout gap — iterate (maspsx `--use-comm-for-lcomm` / `--use-comm-section` are
   the escape hatches).
6. If `-G8` triggers gcc's function-after-data reordering and breaks an
   INCLUDE_ASM order (low risk today), apply maspsx's `# maspsx-keep` /
   `__maspsx_include_asm_hack_<NAME>` workaround in `include/include_asm.h`.
7. **Confirm the version:** decompile a gp-using function (`Think1sleep`
   is the obvious candidate — its C draft is already sketched) and `diff.py` it
   against the target. Escalate the aspsx version only on an observed `la`/`gp`
   mismatch.

## gp globals: making small globals byte-match (SOLVED — per-TU opt-in)

Tenchu addresses some small globals (≤ 8 bytes, within ±32 KB of
`gp = 0x80097698`) via `$gp` (`%gp_rel(SYMBOL)($gp)`), others absolutely
(`lui $at`). **The rule, verified against the original binary: a TU's gp
accesses target only the smalls that TU itself *defines*; externs are always
absolute.** The gp-target addresses per region form disjoint contiguous blocks —
each TU's own `.sdata` — e.g. the think TU's block *starts at*
`FRAMES_UNTIL_END_OF_ALERT` (0x800979c0), which `Think1sleep` reaches via `$gp`,
while the item TU (`ProcItemManebue`, `ProcItemKusuri`) addresses that very same
symbol absolutely and gp-addresses its *own* block (0x80097ac8…) instead. So
ASPSX ignores `.extern SYM,SIZE` for gp decisions — exactly like stock maspsx.

Our decomp complicates this: every symbol is `extern` (addresses come from
`config/symbols` at link), so "defined in this TU" doesn't exist in our sources.
**The fix** ([`nix/maspsx-gp-extern.patch`](../nix/maspsx-gp-extern.patch)): an
opt-in, repeatable **`maspsx --gp-extern SYM`** flag. Listed symbols with a
size-bearing small `.extern SYM,SIZE` (`0 < SIZE ≤ G`; cc1 `-G8` emits these for
every small global it references) are recorded and added to the gp-rewrite
gates; everything else passes through as cc1's one-line macro (`sw $0,SYM`),
which GNU `as` expands to the same `lui $at` sequence ASPSX used. The symbols
are never defined by maspsx, so `R_MIPS_GPREL16` resolves at link to `SYM − _gp`.

Per-file flag lists live in `Build.hs` (`maspsxGpExterns`) and mirror in
`tools/permute.py` (`GP_EXTERNS`): each file lists the smalls its *original* TU
defined — `Think1sleep` → `Me_THINK_C`/`SR`/`Attrib`/`FRAMES_UNTIL_END_OF_ALERT`;
item-TU functions list none. When a new function needs `$gp` for a symbol, that's
the signal its original TU defined it — add it to the list (and it'll seed the
TU-ownership map for the eventual data segments).

History: the first version of this patch gp-addressed **every** small extern
(no flag). That matched `Think1sleep` but was wrong for cross-TU references —
`ProcItemManebue`'s absolute `FRAMES` store (`lui $at`, scheduled late by the
assembler expansion) was unreachable with the blanket patch, since forcing
absolute required declaring the symbol as an unsized array, which makes cc1
materialize the address in a real register (`lui $v0`, scheduled early) instead
of emitting the macro. The opt-in flag keeps cc1's small-data macro emission
(the schedule) *and* lets the assembler pick the original's `$at` expansion.
Note `la` (address-of) is not gp-rewritten at aspsx 2.77 (`gp_allow_la` off),
which is how far `&`-taken strings (`FONT_FILE_NAME`) stay absolute.

## `li` of small positive immediates → `addiu`, not `ori` (SOLVED)

ASPSX **≥ 2.56** (Tenchu is 2.77) loads a signed-16-bit-*positive* immediate with
`addiu $reg,$zero,imm`, **not** `ori` — see maspsx's own
`aspsx/test_expand_li.py` (`EXPAND_LI_RESULT_2`), and GNU `as`'s built-in `li`
does the same. But [`include/macro.inc`](../include/macro.inc) shipped a **custom
`li` macro** (used to assemble maspsx's output, since maspsx passes `li` through
for aspsx ≥ 2.50) whose positive-`< 0x8000` branch emitted `ori` — the pre-2.56
convention. That silently mismatched two bytes in `Think1sleep`
(`addiu $v0,$zero,0x100` vs `ori`, and `0x1001`). Fixed the macro's first branch to
`addiu` (the negative branch was already correct). Only the `0x8000..0xFFFF` case
stays `ori` (there `addiu` would wrongly sign-extend). Any compiled `li` of a
positive constant `≤ 0x7FFF` depends on this.

## Notes on the current `cc1` flags

`ccFlags` is already the standard PSY-Q/maspsx set and should stay: `-mcpu=3000
-G0 -O2 -funsigned-char -fpeephole -ffunction-cse -fpcc-struct-return -fcommon
-fverbose-asm -fgnu-linker -mgas -msoft-float` (only `-G0` → `-G8` changes).
`-mcpu=3000` is harmless — maspsx rewrites it to `-mtune=` for GNU `as`.

## m2c: the overlay is the base, our PS1 patches go on top

`m2c` comes from **`github:Fuuzetsu/m2c-overlay`** (ours). The overlay owns the
upstream `matt-kempster/m2c` pin and resolves m2c's PEP-621 `[project]` deps with
`pyproject-nix`. We do **not** consume its `overlays.default` — that `pkgs.m2c`
is unpatched. Instead `flake.nix` declares

```nix
m2c-src.follows      = "m2c/m2c";
pyproject-nix.follows = "m2c/pyproject-nix";
```

and rebuilds the overlay's own recipe against `pkgs.applyPatches { src = m2c-src;
patches = nix/m2c/*.patch; }`. So the overlay stays the single source of truth for
*which* m2c we use, and our patches ride on top.

**Why we patch at all:** stock m2c has no PS1 GTE/COP2 support, so the ~25
renderer functions decompile to `M2C_ERROR(unknown instruction: lwc2 …)` and
nothing else. The series (13 commits, authored by Patrick Gill, kept as
`git format-patch` files so they stay upstreamable) adds GTE/COP2, `--input-regs`
for the region's non-ABI calling convention, and several correctness fixes
(`swc2` was silently dropping its memory write; PSX `lwr`-before-`lwl` order;
`rfe`/`bltzal`/`bgezal`/`bc0f`/`bc0t`/`bc2f`/`bc2t` no longer abort a function).
m2c's own `run_tests.py` is **393/393** with the series applied (upstream master
alone: 380/380 — the extra tests come from the series).

### Bumping m2c

1. `nix flake update m2c` — this moves the overlay, and `m2c-src` follows it.
2. If upstream moved, `applyPatches` will fail **loudly** at build time. Rebase:

```sh
git clone https://github.com/matt-kempster/m2c && cd m2c
git checkout -b tenchu <new-upstream-rev>
git am /path/to/tenchu-decomp/nix/m2c/*.patch      # fix conflicts if any
python3 run_tests.py                                # must be green
git format-patch --no-signature -o /path/to/tenchu-decomp/nix/m2c <new-upstream-rev>..HEAD
```

3. `nix develop --command m2c.py --help | grep input-regs` as a smoke test, then
   `./Build check`.

Do **not** re-add the `uv.lock`/pytest dev-deps commit that lives on the original
`tenchu` branch: it is 245 KB, irrelevant to the nix build, and irrelevant to
upstream review.

## spimdisasm: we do NOT use `pkgs.spimdisasm`, and bumping it changes nothing

`splat` bundles its own spimdisasm (its `requirements.txt` pins `>=1.13.0`; its
nix env ships **1.13.3**, under python **3.10**). Nothing in this repo imports
spimdisasm — `grep -rn "import spimdisasm" tools/ shake/` is empty. So the
`spimdisasm` overlay input only ever contributed the CLI to the devShell.

**Measured:** bumping `Fuuzetsu/spimdisasm-overlay` to spimdisasm **1.42.2**
leaves the generated asm byte-for-byte identical (415 `.s` files, sha256
`e23e40a6…4bfd` before and after) and `./Build check` byte-identical. There is no
upside — splat never sees it.

**There is a downside, and it broke the build.** The updated overlay exposes
spimdisasm as a `buildPythonApplication`. Putting one in a devShell drags its
site-packages and every propagated dep onto `PYTHONPATH`, which *every*
interpreter honours — including splat's python 3.10. splat then imported our
1.42.2 (built for 3.13) instead of its own 1.13.3, called
`rabbitizer.Utils.escapeString` (an API its bundled rabbitizer 1.7 predates) and
died with:

```
SystemError: PY_SSIZE_T_CLEAN macro must be defined for '#' formats
```

The traceback blames rabbitizer; the cause is the PYTHONPATH hijack. Confirm it
in one line: `env -u PYTHONPATH ./Build check` passes on the bumped pin.

**Fixed upstream in the overlay** (`spimdisasm-overlay`, "Expose only the
executables…"): `packages.spimdisasm` is now a `runCommandLocal` symlinking only
`$out/bin`, so it exports nothing; the library remains as
`packages.spimdisasm-python`. A `spimdisasm-does-not-leak-pythonpath` check
guards it. **Bump `spimdisasm` here only once that fix is pushed** — until then
`nix flake update spimdisasm` reintroduces the hijack. Even after, the bump is
cosmetic; the CLI is the only thing it buys.

## splat 0.41 — MIGRATED (what it changed, and what it needs)

We run **splat 0.41.0** via the rewritten `Fuuzetsu/splat-overlay`, which exports
nothing onto `PYTHONPATH` (see the spimdisasm note above for why that matters)
and keeps a `split.py` shim over the new `splat split` entry point, so `Build.hs`
is unchanged. The image is byte-identical across the bump.

Two config settings are load-bearing:

1. **`config/symbols.main.exe.txt` must have ONE name per address.** splat >= 0.4x
   rejects duplicates unless disambiguated by segment/rom, which our file cannot
   express (it doubles as an ld script — no comment syntax). There were **716
   duplicate names over 181 addresses**, almost all Ghidra
   `switchD_<a>__caseD_<n>` aliases for cases sharing a block.
   `tools/dedupe-symbols.py` drops them, keeping any name our sources reference
   (exactly two). `--check` gates it. `import_symbols.py` only RENAMES, so this
   stays fixed; re-run after a fresh Ghidra import.

2. **`linker_section_order` on the interleaved entries — NOT `ld_legacy_generation`.**
   splat >= 0.4x no longer follows yaml order: it groups linker entries by
   section, emitting all `.text` then all `.data` then all `.rodata`. Our image
   interleaves two runs and cannot be expressed that way directly:
   * `[0x800, 0x5934)` — leading `.data` with six `.rodata` jump-table carves cut
     into it;
   * `[0x5934, 0x87800)` — carved `c` files interleaved with `data` blobs that are
     really **un-carved code** linked as raw bytes (an artifact of incremental
     decompilation, not real data).

   Upstream calls `ld_legacy_generation` "a last resort" that "may be removed in
   the future", and points at **`linker_section_order`** (Segments.md): it
   overrides which ORDER GROUP an entry joins while leaving the section it
   actually links into untouched. So the six leading `.rodata` carves declare
   `linker_section_order: .data`, and the 99 text-region `data` blobs declare
   `linker_section_order: .text`. Each group then holds one contiguous run in
   address order and the script matches the image.

   Three details that cost real debugging:
   * **`.rodata` must still be listed** in `section_order`. splat emits an
     (empty) `.rodata` entry per C object; omitting the section makes generation
     abort with `section '.rodata' not found`. Only our six carves have non-empty
     `.rodata`, so a listed-but-otherwise-empty group costs nothing.
   * **`align: 4` on the segment.** splat pads the END of every section group to
     `segment.align`, which defaults to **16**. The leading run ends at
     `0x80016134` — 4-aligned but not 16 — so the default padded it and shifted
     `.text` by exactly 12 bytes (and grew the file by 16).
   * **The tools must keep emitting the tags.** `reverse.py` splits a `data`
     subsegment on every carve and `split-scaffold.py` inserts `.rodata` carves;
     both now go through `reverse.py`'s `data_sub`/`rodata_sub`, which tag an
     entry from its address (`TEXT_FOFF = 0x5934`). Both parse the dict form too.

**Shake already tracks the generated includes.** The cpp rule does
`need [mainGen, src]`, so splat regenerates `include/` before anything reads it;
cpp's `-MMD` picks up `include_asm.h` and `as --MD` (see `neededAsmDeps`) picks up
`macro.inc` and, through it, `gte_macros.inc`. Verified: perturbing
`generated_macro_inc_content` rebuilds 104 steps and `./Build check` stays green.

**splat now GENERATES `include/{macro.inc,include_asm.h,labels.inc,gte_macros.inc}`
on every run**, overwriting the first two. That silently dropped our ASPSX `li`
override — the image still matched only because GNU `as`'s own `li` picks the same
encoding, which is luck, not correctness. The overrides (`li`, `move`, `.def`) are
re-injected through the yaml's **`generated_macro_inc_content`**. The four files
are regenerated deterministically and committed, so a rebuild leaves the tree
clean; if a splat bump changes them, the diff is visible in review.

Two consequences worth knowing:

- **`gte_macros.inc` (68 macros) is why the GTE region assembles.** splat emits
  `rtpt`/`nclip`/`dpcs` as lowercase mnemonics that expand through those macros.
  Our stopgap `tools/gte2word.py` is deleted.
- **Instruction lines are now indented** (`    /* off vram WORD */  insn`) and
  functions gain `nonmatching NAME, SIZE` / `endlabel NAME` wrappers. Anything
  anchored on `^/\*` breaks. `tools/permute.py`'s `piece_addr` was the dangerous
  one: it returned a sentinel on no-match, which would have silently MISORDERED
  jump-table pieces rather than failing. It now uses `re.search`.

`tools/reverse.py` still auto-seeds `__override__prt_` two-piece splits; note
splat's generated stub now spells the folder `.shake/gen/...` rather than
`config/../.shake/...`, and both forms assemble.
