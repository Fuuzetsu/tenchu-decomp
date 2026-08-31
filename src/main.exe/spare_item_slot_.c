#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * spare_item_slot_ (0x8004a368, 0x80 bytes) — get/set accessor over the LAST
 * slot of Humanoid's per-item-kind count array (item[ITEM_N] — the same
 * index DoInfoViewProc's cursor wraps at, i.e. this repurposes the array's
 * spare slot as a plain flag, not a real item count): mode 0 clears it,
 * mode 1 reports whether it's == 1, anything else complains via
 * AdtMessageBox. A NULL humanoid arg defaults to the current camera owner,
 * `CamState.Owner`.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - Ghidra's `field_0xcd`/m2c's `unkCD` is item.h's proven `item[0x1A]`
 *    array at Humanoid+0xB4: 0xCD - 0xB4 = 0x19, its LAST element — trust
 *    the shared proven struct over Ghidra's invented field name.
 *  - `human->item[ITEM_N] == 1` is the standard MIPS no-`seq`-instruction
 *    equality idiom (`xori`, `sltiu`), not anything hand-shaped.
 *  - It really is a `switch` (measured byte-identical 2026-08-31; the
 *    earlier "explicit goto-ladder, NOT a switch" note here was wrong,
 *    and m2c's "irregular switch" rendering was right). cc1 compiles
 *    this three-way dispatch to tests in source order (0, 1, default)
 *    while placing the default's `jal AdtMessageBox` physically AFTER
 *    both case bodies; only the dependency-free argument address
 *    (`lui`/`addiu` of fmt_not_support_yet) is scheduled early, split
 *    across the preceding `beq`'s delay slot and the jump to the
 *    default body, which reorg retargets past both stolen instructions
 *    straight to the `jal`.
 *  - The single trailing `return 0;` after the switch (not one per
 *    falling case) pins the shared zero-return tail deterministically —
 *    cross-jump merging two independent `return 0;` statements is not
 *    guaranteed (cookbook:
 *    "Multiple return CONST; statements cross-jump unpredictably; a
 *    labeled return body pins them").
 *  - case0 needs a FRESH local (`Humanoid *p = human;`) rather than
 *    reassigning the parameter — it ends up hard-allocated to $v0 (dead by
 *    the time `ret0:` reuses $v0 for the return value), whereas case1
 *    keeps reassigning `human` itself (stays in $a1, its parameter home).
 *    Both fallbacks still use the shared `CamState.Owner` field; the distinct
 *    local identities are enough to produce the target's register forms.
 */

extern void AdtMessageBox(char *fmt, ...);
extern char fmt_not_support_yet[]; /* not support yet %d */

s32 spare_item_slot_(s32 mode, Humanoid *human)
{
    enum
    {
        SPARE_ITEM_SLOT_CLEAR = 0,
        SPARE_ITEM_SLOT_QUERY = 1
    };
    switch (mode)
    {
    case SPARE_ITEM_SLOT_CLEAR:
    {
        Humanoid *p = human;
        if (p == 0)
            p = CamState.Owner;
        p->item[ITEM_N] = 0;
        break;
    }
    case SPARE_ITEM_SLOT_QUERY:
        if (human == 0)
            human = CamState.Owner;
        return human->item[ITEM_N] == 1;
    default:
        AdtMessageBox(fmt_not_support_yet, mode);
        break;
    }
    return 0;
}
