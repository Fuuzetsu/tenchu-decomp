#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * FUN_8004a368 (0x8004a368, 0x80 bytes) — get/set accessor over the LAST
 * slot of Humanoid's per-item-kind count array (item[0x19] — the same
 * index DoInfoViewProc's cursor wraps at, i.e. this repurposes the array's
 * spare slot as a plain flag, not a real item count): mode 0 clears it,
 * mode 1 reports whether it's == 1, anything else complains via
 * AdtMessageBox. A NULL humanoid arg defaults to the currently-controlled
 * character (CURRENTLY_SELECTED_CHARACTER_STATE_PTR — same absolute
 * address as CamState.Owner; ProcItemLightningBolt's header documents the
 * two independently-named aliases of 0x80089f00).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - Ghidra's `field_0xcd`/m2c's `unkCD` is item.h's proven `item[0x1A]`
 *    array at Humanoid+0xB4: 0xCD - 0xB4 = 0x19, its LAST element — trust
 *    the shared proven struct over Ghidra's invented field name.
 *  - `arg1->item[0x19] == 1` is the standard MIPS no-`seq`-instruction
 *    equality idiom (`xori`, `sltiu`), not anything hand-shaped.
 *  - Explicit goto-ladder, NOT nested if/else (cookbook's "if (cond) goto L;
 *    ladder decouples test ORDER from body LAYOUT"): all 3 tests
 *    (`beqz`/`beq`/fallthrough) fire in source order (0, 1, default), but
 *    the DEFAULT case's actual `jal AdtMessageBox` sits physically AFTER
 *    both case bodies — only its dependency-free argument address
 *    (`lui`/`addiu` of D_80012224) gets scheduled early, split across the
 *    preceding `beq`'s delay slot and the ladder's own unconditional
 *    `goto do_default`, which reorg retargets past both stolen
 *    instructions straight to the `jal` (same delay-slot-stealing
 *    mechanism as a shared-tail call, cookbook's "Shared tails" section).
 *    m2c's "irregular switch" rendering is its own recovery heuristic over
 *    this same goto shape, not evidence of a real `switch`.
 *  - `goto ret0;`/`ret0: return 0;` (not two separate `return 0;`) pins the
 *    shared zero-return tail deterministically — cross-jump merging two
 *    independent `return 0;` statements is not guaranteed (cookbook:
 *    "Multiple return CONST; statements cross-jump unpredictably; a
 *    labeled return body pins them").
 *  - case0 needs a FRESH local (`Humanoid *p = arg1;`) rather than
 *    reassigning the parameter — it ends up hard-allocated to $v0 (dead by
 *    the time `ret0:` reuses $v0 for the return value), whereas case1
 *    keeps reassigning `arg1` itself (stays in $a1, its parameter home).
 *    Both read identically in C; only the register outcome differs.
 *  - CURRENTLY_SELECTED_CHARACTER_STATE_PTR and D_80012224 are both
 *    ABSOLUTE (lui/lw, lui/addiu) in this TU — no %gp_rel operands at all
 *    in the target asm, so no gp-extern entry is needed here.
 *
 * STATUS: NON_MATCHING — 2 of 128 bytes differ. Residual is case1's
 * null-guard reload of CURRENTLY_SELECTED_CHARACTER_STATE_PTR: the target
 * computes the symbol's %hi into a SCRATCH $v0, then loads %lo(...)($v0)
 * directly into $a1 (arg1's own fixed register); our cc1 computes both
 * halves directly into $a1. This is the cookbook's named
 * la/address-materialization reload tie ("%hi in a temp vs the target
 * reg") — PrepareAccess (2 bytes) and cd_open (4 bytes) are the identical
 * tie, both confirmed permuter-immune; per the early-stop rule this is
 * NOT permuted here. Tried: giving case1 the same fresh-local shape as
 * case0 (which DOES need the $v0 split) — no change, confirming the
 * residual isn't a statement-shape choice but a reload-pass register pick
 * with no source lever.
 */

extern Humanoid *CURRENTLY_SELECTED_CHARACTER_STATE_PTR;
extern void AdtMessageBox(char *fmt, ...);
extern char D_80012224[]; /* "not support yet %d" */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8004a368", FUN_8004a368);
#else
s32 FUN_8004a368(s32 arg0, Humanoid *arg1)
{
    if (arg0 == 0)
        goto case0;
    if (arg0 == 1)
        goto case1;
    goto do_default;

case0:
    {
        Humanoid *p = arg1;
        if (p == 0)
            p = CURRENTLY_SELECTED_CHARACTER_STATE_PTR;
        p->item[0x19] = 0;
    }
    goto ret0;

case1:
    if (arg1 == 0)
        arg1 = CURRENTLY_SELECTED_CHARACTER_STATE_PTR;
    return arg1->item[0x19] == 1;

do_default:
    AdtMessageBox(D_80012224, arg0);
ret0:
    return 0;
}
#endif
