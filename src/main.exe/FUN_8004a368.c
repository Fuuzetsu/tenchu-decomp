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
 *  - **case1's reload of the currently-selected character MUST be spelled
 *    `CamState.Owner` (the nonzero +0x10 struct-member load), NOT the
 *    offset-0 alias `CURRENTLY_SELECTED_CHARACTER_STATE_PTR`, even though
 *    they are literally the same address (0x80089f00).** Under
 *    -msplit-addresses, the offset-0 alias folds `%hi` into the
 *    destination register (`lui a1,%hi; lw a1,%lo(a1)` — what case0's
 *    `p` reload correctly wants, and what this residual WRONGLY produced
 *    when case1 also used the alias), while the +0x10 struct-member load
 *    splits the base into a separate scratch register (`lui v0,%hi
 *    (CamState); lw a1,0x10+%lo(v0)`), which is the target's actual
 *    2-byte-different reload. Same lever as CheckCheatCodes's
 *    `CamState.Owner` requirement; confirms the rule generalizes to a
 *    SECOND function reaching the SAME 0x80089f00 cell two different ways
 *    in the SAME source file (case0 via the alias, case1 via the
 *    struct-member) — the earlier NON_MATCHING note calling this
 *    "permuter-immune with no source lever" was wrong; a 42k-iteration
 *    permuter run also plateaued flat at the baseline score, confirming
 *    this class needs the alias/struct-member lever, not permutation.
 */

extern Humanoid *CURRENTLY_SELECTED_CHARACTER_STATE_PTR;
extern TCameraStatus CamState;
extern void AdtMessageBox(char *fmt, ...);
extern char D_80012224[]; /* "not support yet %d" */

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
        arg1 = CamState.Owner;
    return arg1->item[0x19] == 1;

do_default:
    AdtMessageBox(D_80012224, arg0);
ret0:
    return 0;
}
