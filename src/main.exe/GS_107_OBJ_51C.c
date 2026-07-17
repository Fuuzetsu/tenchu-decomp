#include "common.h"
#include "main.exe.h"

/*
 * GS_107_OBJ_51C (0x80065b00) — stock PsyQ libgs: read back the cached
 * light-colour matrix. The exact mirror of GS_107_OBJ_4B8 (which writes `_LC`
 * and loads the GTE); this one just copies `_LC` out into the caller's MATRIX,
 * so it needs no frame and ends with the copy's last store in the `jr ra` delay
 * slot.
 *
 * Names are Ghidra placeholders (object 107, offset 0x51C) — not renamed here;
 * a rename needs callmatch --verify and its own commit.
 */
extern MATRIX _LC;

/*
 * STATUS: NON_MATCHING — ONE INSTRUCTION over (20 vs 19). Every other
 * instruction, register and operand is exact, and it needs
 * `-mno-split-addresses` (Build.hs ccExtraFlags) to get there — same provenance
 * as its sibling GS_107_OBJ_4B8.
 *
 * THE RESIDUAL IS SHARED WITH GS_107_OBJ_4B8, and it is the same instruction in
 * both: reorg will not put the struct-copy's LAST STORE into the branch's delay
 * slot. Here the target ends `jr ra / sw v1,28(a0)`; we emit the store, then
 * `jr ra`, then a `nop`.
 *
 * cc1 SAYS SO ITSELF — this is measured, not inferred
 * (`tools/cc1says.py GS_107_OBJ_51C --pass dbr`):
 *
 *     ;; Reorg pass #1:
 *     ;; 1 insns needing delay slots
 *     ;; 1 got 0 delays
 *
 * So reorg TRIED and failed; it is not that the slot was never a candidate. The
 * store touches none of `jr ra`'s operands and sits directly above it, so plain
 * eligibility is not the blocker.
 *
 * WORKING HYPOTHESIS (untested): `*m = _LC;` is a whole-struct assignment, which
 * cc1 expands as a BLOCK MOVE — and compiler-facts records that
 * `fill_simple_delay_slots` stops at SEQUENCEs. If the 32-byte copy is one RTL
 * insn emitting many assembly instructions, reorg cannot take just its tail. The
 * target filling the slot would then mean the ORIGINAL did not write a struct
 * assignment here — it wrote something that produces separate insns. The
 * load/store batching (3+3, 3+3, 2+2) is characteristic of the block move
 * though, so that cuts the other way; check the RTL before believing either.
 * Fixing this once fixes BOTH functions.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GS_107_OBJ_51C", GS_107_OBJ_51C);
#else
void GS_107_OBJ_51C(MATRIX *m)
{
    *m = _LC;
}
#endif
