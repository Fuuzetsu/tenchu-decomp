# Toolchain: maspsx vs ASPSX.EXE, and how to reach byte-matches

> **Status: maspsx is integrated.** The pipeline is `cpp | cc1-281 -G8 | maspsx
> --aspsx-version=2.77 -G8 | as -G0 | ld`. wine has been removed from the
> devShell. `./Build check` stays byte-identical. The "Recipe" section below is
> kept as the rationale/record.

## TL;DR — do we need maspsx or ASPSX.EXE / wine?

**Use maspsx. Delete wine.** (Done.)

maspsx *replaces* `ASPSX.EXE` (+ `psyq-obj-parser`); it is not a complement. This
repo already committed to the modern, wine-free "path B" that every active PSX
decomp uses (SOTN, Silent Hill, MediEvil, Soul Reaver, Croc, Spyro):

| PSY-Q tool (path A, needs wine/DOS) | This repo (path B, native Linux) |
|---|---|
| `CC1PSX.EXE` (compiler)             | `tools/cc1-281` (GCC 2.8.1 cross-`cc1`) |
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

## Recipe: add maspsx to the pipeline (done — kept as the record)

This is how maspsx was integrated (all steps below are done except cc1 pinning,
which stays on the roadmap). Gating check: `./Build check` stayed byte-identical,
and `get_held_buttons`/`initialise_font` were verified unchanged under `-G8` +
maspsx *before* the flag flip.

1. **Vendor maspsx** into the devShell (it's a single-file, stdlib-only Python
   script). Mirror the `asm-differ` pattern in `flake.nix`: add a
   `fetchFromGitHub mkst/maspsx` (or a Fuuzetsu-style overlay if one exists) and
   a `writeShellScriptBin "maspsx"` that runs `python3 maspsx.py`. In the same
   edit, **delete `pkgs.wine`**.
2. **Pin `cc1-281` first.** Verify the committed `tools/cc1-281` bytes match a
   specific [decompals/old-gcc](https://github.com/decompals/old-gcc) release
   (`gcc-2.8.1-psx.tar.gz`), then replace the opaque binary with a `fetchurl`
   pinned to that sha256. Byte-identical codegen depends entirely on this exact
   `cc1`.
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
7. **Confirm the version:** decompile a gp-using function (`think_setting_sleep`
   is the obvious candidate — its C draft is already sketched) and `diff.py` it
   against the target. Escalate the aspsx version only on an observed `la`/`gp`
   mismatch.

## gp globals: making small globals byte-match (important)

Tenchu addresses small globals (≤ 8 bytes, within ±32 KB of `gp = 0x80097698`)
via `$gp` — the disassembly shows `%gp_rel(SYMBOL)($gp)`. To reproduce that from
compiled C you must get maspsx to rewrite the access to `%gp_rel`. **maspsx only
gp-converts symbols it sees declared as small-data** (`.comm`/`.lcomm`/`.sdata`/
`.sbss`); it *ignores* `.extern`.

So a small global declared `extern s16 ALERT_STATUS_;` compiles to an **absolute**
`lui/…` access and will **not** match. Instead declare it as a **tentative
definition** (drop `extern`):

```c
s16 ALERT_STATUS_;      /* not `extern` → cc1 emits `.comm ALERT_STATUS_,2` */
```

Then, with `-fcommon` (already set) + cc1 `-G8`:
- cc1 emits `.comm ALERT_STATUS_,2`;
- maspsx sees the small `.comm`, tracks it, and rewrites `sh $2,ALERT_STATUS_`
  → `sh $2,%gp_rel(ALERT_STATUS_)($gp)`;
- at link, the absolute definition from `undefined_symbols_auto.main.exe.txt`
  (`ALERT_STATUS_ = 0x800979D6;`) overrides the common symbol, so nothing is
  allocated and `%gp_rel` resolves to `addr - gp` — matching the target.

This is verified working (`sh $2,ALERT_STATUS_` → `%gp_rel(...)` under
`--aspsx-version=2.77 -G8`). Big/far globals (e.g. `HELD_BUTTONS` at `0x800be6d0`,
outside gp range) stay `extern` and are addressed absolutely — that's correct.

The clean long-term form is to split these globals into a real `.sdata`/`.sbss`
section symbolically; the tentative-definition trick is the interim that matches
today. `think_setting_sleep` is the first function that needs this.

## Notes on the current `cc1` flags

`ccFlags` is already the standard PSY-Q/maspsx set and should stay: `-mcpu=3000
-G0 -O2 -funsigned-char -fpeephole -ffunction-cse -fpcc-struct-return -fcommon
-fverbose-asm -fgnu-linker -mgas -msoft-float` (only `-G0` → `-G8` changes).
`-mcpu=3000` is harmless — maspsx rewrites it to `-mtune=` for GNU `as`.
