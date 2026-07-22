#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * enum TItemType GetItemType(int ConflictID);
 *     ITEM.C:601, 12 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       int ConflictID
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * GetItemType (0x8004a3e8, 0x44 bytes) — linear-search the global item pool
 * `items[]` (item.h's proven tag_TItem; 30 slots, the same bound
 * ClearItemLayout.c uses) for the slot whose `locate->id` (ModelType.id)
 * matches ConflictID, returning that slot's `type`. Falls back to
 * ITEM_KAGINAWA if none of the 30 slots match before the count runs
 * out. `i` counts iterations in $a1, `p` walks `items` in $v1; i's increment
 * is the first body statement (falls into the mismatch branch's delay slot)
 * and p's advance is the last (the back-branch's delay slot).
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - This LOOKS like a "guard clause with two returns" (Ghidra: `if (match)
 *    return type; ...keep searching...; return FALLBACK;`), which the
 *    cookbook's null-guard exception says to keep in Ghidra's literal
 *    polarity. That's wrong here and relocates the match-return to the far
 *    end of the function (past the fallback), because the "rest" arm
 *    contains the loop's OWN back-edge (`goto loop`) — cc1 keeps the code
 *    lexically AFTER the if as the inline fallthrough and relocates the
 *    if-body, regardless of which side is the "guard". Fix: invert the
 *    condition and NEST the keep-searching-and-fall-back logic inside the
 *    if, leaving the found-return as the final, unnested statement — see the
 *    refined rule added to the cookbook's Dispatch section.
 */

TItemType GetItemType(s32 ConflictID)
{
    tag_TItem *p;
    s32 i;

    i = 0;
    p = items;
loop:
    i++;
    if (p->locate->id != ConflictID)
    {
        p++;
        if (i < 0x1e)
        {
            goto loop;
        }
        return ITEM_KAGINAWA;
    }
    return p->type;
}
