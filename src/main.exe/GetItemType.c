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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int ConflictID
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * GetItemType (0x8004a3e8, 0x44 bytes) — linear-search the global item pool
 * `items[]` (item.h's proven TItem; 30 slots, the same bound
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
    TItem *p;
    s32 i;

    i = 0;
    p = items;
loop:
    i++;
    if (p->locate->id != ConflictID)
    {
        p++;
        if (i < MAX_ITEMS)
        {
            goto loop;
        }
        return ITEM_KAGINAWA;
    }
    return p->type;
}
