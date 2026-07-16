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
5. Additions to the whitelist are an owner-level decision. In particular, the
   `GetPad`/`GetPadXY`/`FUN_8001b174` sign-extension trio and `PClseek` stay
   OFF the list deliberately: their originals were almost certainly plain C
   (a different compiler fold / a libsn assembly object), so asm bodies there
   would be unfaithful. They remain parked NON_MATCHING.

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

The drawF3 anchor is 72/74 instructions exact. An early investigation called
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
One diagnostic inline `li` at the reset makes drawF3 byte-exact, but it is an
ordinary MIPS instruction rather than a GTE operation and is not retained or
hidden inside `gte.h`.

Scanning every whitelist function for the `addiu r,$zero,0` tell splits the
family cleanly:

- **HANDWRITTEN (17)**: all 16 `draw*` renderers (exactly 1 tell each — the
  same `code = 0` site drawF3 could not reach) and `DrawTMD` (2 tells). Their
  original source was assembly; a C reconstruction is documentation, not a
  faithful source form. The non-ABI dispatcher also calls each handler with
  live state in `$v0/$t0/$t2-$t6/$t9/$s0`, without marshalling C arguments;
  the handlers return updated state in those same registers. drawF3's
  5-differing-byte NON_MATCHING draft (72/74 exact) is kept as the documented
  reference reconstruction; DO NOT clone the other 15 as ~8-byte parked drafts
  — the owner decides the class's accounting
  (committed-canonical-asm vs excluded) before any further lanes.
- **COMPILED-STYLE (8)**: `SetDepthQ` (matched), the twin pairs
  `FUN_80058c70/FUN_80059008` (920 B), `FUN_80059ff4/FUN_8005a3cc` (984 B),
  `FUN_8005961c/FUN_80059b08` (1260 B), and `FUN_80057b80` (3796 B) — 10,084
  unmatched bytes where the gte.h layer is exactly the original mechanism.

## Matching order for the family (revised)

1. ~~`SetDepthQ`~~ — DONE (the spike).
2. ~~`drawF3`~~ — anchor DONE as the documented 5-differing-byte reconstruction;
   family conventions + macro set landed in `gte.h`.
3. `FUN_80057b80` (3796 B, 2 GTE commands — mostly ordinary C).
4. The three twin pairs: match one anchor per pair, clone its twin.
5. ~~The 15 remaining `draw*` + `DrawTMD`~~ — RESOLVED (owner decision,
   2026-07-16): the class is **asm-canonical**. Their assembly is the faithful
   source form (scene-standard, like SM64's handwritten `.s`);
   `config/handwritten-asm.txt` is the machine-readable list, `progress.py`
   counts them in the `game done (C+asm)` line, and `triage.py` hides them
   from targets permanently. drawF3's 5-differing-byte C reconstruction
   remains as documentation; no further C lanes for this class.
