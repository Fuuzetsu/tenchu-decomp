# The restricted GTE inline-asm policy

Adopted 2026-07-16 by the project owner. This resolves the long-standing
"inline-asm policy" question that parked the GTE/COP2 class (previously
~22% of the remaining unmatched game bytes).

## The decision

Inline assembly is allowed in matched source **only** through the shared macro
header [`src/main.exe/gte.h`](../src/main.exe/gte.h), and **only** in functions
listed in [`config/gte-allowlist.txt`](../config/gte-allowlist.txt). Everywhere
else, `__asm__` in a draft remains an automatic failure
(`tools/matchdiff.py` `source_completion_blockers`), exactly as before.

Rationale: cc1 2.8.1 has **no C construct that emits a COP2 opcode**, and the
`draw*` render handlers additionally **receive their arguments in
`$t2..$t5`/`$s0`**, outside the ABI. The original developers did not write
these in C either — PsyQ's own `libgte` interface (`INLINE_N.H`/`inline_c.h`)
is a layer of inline-asm macros (`gte_ldDQA()`, `gte_rtps()`, …). A restricted
macro header reconstructing that API **is** the faithful decompilation of the
original source, the same way N64 decomps accept GBI macros. It is not a
matching shortcut.

## Rules

1. `__asm__` may appear in exactly ONE file under `src/`: `gte.h`. A unit test
   (`tools/tests/test_matching_tools.py`) enforces this containment, and that
   every `.c` including `gte.h` is on the allowlist.
2. `gte.h` contains only:
   - COP2 data moves (`mtc2`/`mfc2`/`ctc2`/`cfc2`/`lwc2`/`swc2`) behind
     PsyQ-named macros (`gte_ldDQA`, `gte_ldv0`, `gte_stsxy`, …);
   - GTE command issues as `.word 0x4A......` with the mnemonic in a comment
     (`gte_rtps`, `gte_nclip`, …). Use `.word`: the `.c` pipeline's `as` input
     does not include splat's `gte_macros.inc`, and `.word` is
     assembler-agnostic. NOTE splat's `/* addr vaddr WORD */` comment column in
     carved `.s` is the little-endian FILE bytes, not the instruction word —
     `RTPT` prints `3000284A` and encodes as `.word 0x4A280030`.
   - Nothing with side effects a C statement could express instead.
3. Macros take/return values through `"r"`/`"=r"` constraints. Where a
   function's entry convention is non-ABI (the `DrawTMD` handler family),
   register-pinned locals are allowed in the whitelisted `.c` itself:
   `register long p __asm__("$12");` (verified working in this cc1 — see the
   PClseek.c investigation header).
4. New macros: mirror the real `INLINE_N.H` name if one exists; otherwise use
   the PsyQ naming style. Matcher agents may extend `gte.h` when their target
   needs a missing operation, and must report the addition for review.
5. Additions to the whitelist are an owner-level decision. The whitelist is
   only for genuine COP2/GTE operations; unrelated integer or support-code
   residuals do not belong here. `GetPad`, `GetPadXY`, and `FUN_8001b174` are
   now exact plain C through the encoded-port idiom, confirming that excluding
   them from inline assembly was correct. `PClseek` remains a separate
   support-code investigation, not a GTE-policy precedent.

## Verified pipeline facts (the SetDepthQ spike, 2026-07-16)

- cc1 2.8.1 passes extended-asm blocks through as `#APP`/`#NO_APP` regions;
  operands with `"r"` constraints allocate normally (a0/a1 arrived unmoved).
- maspsx treats the `#APP`/`#NO_APP` markers as non-instructions and passes
  unknown COP2 mnemonics through untouched (its transforms key on specific
  load/store/li/la/div opcodes).
- GNU `as` (binutils 2.40, `-march=r3000`) assembles the COP2 data moves
  natively; only GTE *command* mnemonics need the `.word` form in `gte.h`.
- Result: `SetDepthQ` (2 × `ctc2`) matched byte-exactly on the first build,
  whole image identical.
- Volatile asm blocks are scheduling barriers for cc1 — faithful, since the
  original macros were equally opaque to the original compiler.

## The whitelist (initial, 25 functions)

`config/gte-allowlist.txt` is the machine-readable copy; it starts as:
`SetDepthQ` (matched), `DrawTMD`, the 16 primitive renderers
`drawF3 drawG3 drawzF3 drawF4 drawFT3 drawzG3 drawG4 drawGT3 drawzFT3
drawzF4 drawFT4 drawzGT3 drawzG4 drawGT4 drawzFT4 drawzGT4`, the twin pairs
`FUN_80058c70/FUN_80059008`, `FUN_80059ff4/FUN_8005a3cc`,
`FUN_8005961c/FUN_80059b08`, and `FUN_80057b80`.

## Compiled vs HANDWRITTEN split (established by the drawF3 anchor, 2026-07-16)

The best pure-C drawF3 anchor reached 72/74 instructions. An early investigation called
both residual sites cc1-2.8.1 invariants. The equality claim was too strong:
computing `0x304 - input_v0` in the pinned `$v1` carrier and testing that result
for zero lets combine emit the target's `beq $v1,$v0` with no subtraction left.
That instruction is therefore reachable from C.

The zero materialization remains a real backend boundary. An integer `x = 0`
uses cc1's movsi form, `move r,$0`; the target uses `addiu r,$zero,0`
(`li r,0`). This held for ordinary and fixed-register carriers, varied source
expressions, cc1 2.6.3 and 2.8.1, and every current cc1-produced assembly file.
The two words still differing in the best pure-C draft are coupled scheduling
effects around that one reset: using `$s0` as the comparison RHS keeps the
`move` between `cfc2` and `and` and preserves the target's delay-slot `nop`.
The final guarded reference is 74/74 exact. Rather than inventing a fake C
dependency, it keeps the original adjacent `cfc2 flag,$31; addiu code,$zero,0`
and literal flag test as one coherent local handwritten unit. It is deliberately
not hidden inside `gte.h`; the canonical assembly guard remains because the
function's provenance is assembly.

Scanning every whitelist function for the `addiu r,$zero,0` tell splits the
family cleanly:

- **HANDWRITTEN (17)**: all 16 `draw*` renderers (exactly 1 tell each — the
  same `code = 0` site drawF3 could not reach) and `DrawTMD` (2 tells). Their
  original source was assembly; a C reconstruction is documentation, not a
  faithful source form. The non-ABI dispatcher also calls each handler with
  live state in `$v0/$t0/$t2-$t6/$t9/$s0`, without marshalling C arguments;
  the handlers return updated state in those same registers. drawF3's
  byte-exact guarded reference is kept as documentation; DO NOT clone the other
  15 into pretend-C matching targets — the owner decides the class's accounting
  (committed-canonical-asm vs excluded) before any further lanes.
- **COMPILED-STYLE (8, now all matched)**: `SetDepthQ`, the twin pairs
  `FUN_80058c70/FUN_80059008` (920 B), `FUN_80059ff4/FUN_8005a3cc` (984 B),
  `FUN_8005961c/FUN_80059b08` (1260 B), and `FUN_80057b80` (3796 B). The
  restricted gte.h layer reproduced the original compiled mechanism for all
  eight.

## Matching order for the family (revised)

1. ~~`SetDepthQ`~~ — DONE (the spike).
2. ~~`drawF3`~~ — provenance anchor DONE; guarded reference is byte-exact.
3. ~~`FUN_80057b80`~~ — DONE.
4. ~~The three twin pairs~~ — all six DONE.

**Two facts this family established** (detail in the cookbook):
- **Latency nops are provenance-keyed.** `gte_<cmd>()` carries `nop;nop` (what a
  compiled caller's PsyQ macro emitted); `gte_<cmd>_raw()` is bare, for
  reconstructions of handwritten assembly whose authors scheduled the latency by
  hand. A unit test pins handwritten files to `_raw` — the nop form silently put
  drawF3 24 bytes over its carve, and only the linked length caught it.
- **The permuter cannot search this class**: its C parser rejects inline asm, so
  `permute.py` refuses gte.h functions outright and points at RTL escalation.
5. ~~The 15 remaining `draw*` + `DrawTMD`~~ — RESOLVED (owner decision,
   2026-07-16): the class is **asm-canonical**. Their assembly is the faithful
   source form (scene-standard, like SM64's handwritten `.s`);
   `config/handwritten-asm.txt` is the machine-readable list, `progress.py`
   counts them in the `game done (C+asm)` line, and `triage.py` hides them
  from targets permanently. `drawF3` now also has a byte-exact guarded
  reference reconstruction using one coherent handwritten unit; the default
  canonical assembly remains authoritative. No further pure-C lanes for this
  class.

## Post-demo handwritten Hermite helper (owner decision, 2026-07-19)

`FUN_8001c730` is also **asm-canonical**, independently of the 17-function draw
family. The demo's `GetSpline` (0x8001b39c) evaluates all four Hermite terms in
scalar C. Retail replaces that exact tail with a call to this new 220-byte leaf:
it feeds Hermite terms 0..2 and the `key0/key1/dd0` matrix through GTE MVMVA,
then hand-interleaves the three `ds1 * term3` multiplies with MAC1..3 reads.
The helper is absent from the symbol-bearing demo (which records static
functions too) and sits at the end of retail's ACTION.C run, immediately before
the first MOTION.C function.

The compiler investigation used debug RTL rather than mismatch-count folklore.
Human-shaped direct-field C reproduces every value and operation. Register pins
can make the complete prefix through the first multiply exact, but only by
writing the target's `$t0..$t5` plan into C. The tail still requires a custom
nonvolatile MAC-to-register asm block to cross the third `mflo`; the plausible
volatile spelling orders that `mflo` before the `mfc2` reads. Real PsyQ
`INLINE_C.H` has `gte_rtv0_b()`, but its result macro `gte_stlvnl()` stores with
`swc2` and has no MAC1..3-to-C-register form. In this image, those register-form
MAC reads otherwise occur only in the already-classified handwritten draw
handlers. Keeping fixed-register C or inventing a fake GTE macro would therefore
encode assembly as C. The semantic C reference remains in
`src/main.exe/FUN_8001c730.c`; `config/handwritten-asm.txt` makes the original
assembly authoritative.

## Candidate for the handwritten class: PClseek (flagged, NOT yet added)

`PClseek` (0x80060224, SDK region) is almost certainly handwritten assembly, on
the same principle that classified the GTE 17 — but it is **not** in
`config/handwritten-asm.txt` yet, because membership there is attributed to an
explicit owner decision. Flagged for ratification. The evidence is three
independent tells, the first quantitative:

* **`break 0, 263` (0x107) is the ONLY two-operand break in the whole image.**
  cc1's own trap codes appear in bulk — `break 7` (divide-by-zero) 139 times,
  `break 6` (overflow) 131 times — because they come from `mips.md`. 263 belongs
  to no cc1 pattern; it is the dev kit's host-link trap code. **A compiler does
  not emit a code it does not own.** This is a clean general tell: survey the
  trap codes, and the ones that appear once are not compiler output.
* **The args are shifted UP one register** (`$a0->$a1`, `$a1->$a2`, `$a2->$a3`) so
  the trap handler can read them from `$a1-$a3`. No C calling convention produces
  that.
* **`$v1` is read as a SECOND return value** beside `$v0` (error code / result),
  inexpressible in the MIPS C ABI — and the target's own branch label is
  `LSEEK_OBJ_1C`, i.e. the original object's label survived into the image.

The guarded reference in `src/main.exe/PClseek.c` is now byte-exact (0/36), and
the demo's independently linked body is also 36/36 identical with no C TU/line
records. It expresses the trap, private-ABI branch, success-result delay-slot
copy, and error result as one handwritten unit; the default assembly remains
canonical pending manifest ratification. The same correction may apply to its
siblings (`PCopen`, `PCcreat`, `PCclose`,
`PCread`, `PCwrite` — all in the same PsyQ host-file-I/O family, all in the SDK
region above 0x80060000, none of which affect the game-code scoreboard).
