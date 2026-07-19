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
 * The sibling's compiler dump explains this residual too: GCC 2.8.1 leaves the
 * global side as a symbolic-address `movstrsi_internal`, while its delay-slot
 * split requires register addresses. The exact stock GS_107.OBJ was compiled by
 * the SDK's different compiler and proves that the natural whole-struct copy is
 * the right source shape. Fix the original-object compiler profile, not this
 * function with manual copies; doing so fixes BOTH leaves.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GS_107_OBJ_51C", GS_107_OBJ_51C);
#else
void GS_107_OBJ_51C(MATRIX *m)
{
    *m = _LC;
}
#endif
