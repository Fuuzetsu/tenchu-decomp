#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ClearItemLayout(void);
 *     ITEM.C:461, 8 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * ClearItemLayout (0x8004a500) — debug menu "clear item layout" action
 * (DoInfoViewProc's ItemLayoutMenu, case 1, after the "clear ok?" confirm
 * comes back true). Force-disposes every live slot in the item pool: for
 * each of the 30 slots with proc != 0, run its proc with
 * mode=ITEM_MODE_DISPOSE, delete
 * the conflict, complain if mode didn't clear, then clear owner/proc — the
 * identical dispose sequence as ReqItemDrop/ProcItemManebue/GetFreeItemSlot.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - A hand-rolled `label: ...; goto label;` loop, NOT a `for`/`while`: the
 *    target keeps ONE cursor register (`it`) with plain field offsets and
 *    never hoists the loop-invariant D_800121CC address or the 0xff
 *    constant out of the loop. A real for/while(1) here gets recognized by
 *    loop.c, which both strength-reduces `it->proc`'s repeated offset into a
 *    SECOND derived pointer (a giv alongside `it` itself) and hoists the
 *    invariants to a preheader — neither happens in the target, so no loop
 *    notes were ever emitted (front end never sees a real loop construct).
 */

void ClearItemLayout(void)
{
    TItem *it;
    s32 i;

    i = 0;
    it = items;
loop:
    if (!(i < 0x1e))
        goto end;
    if (it->proc != 0)
    {
        it->mode = ITEM_MODE_DISPOSE;
        it->proc(it);
        DeleteConflict(it->locate);
        if (it->mode != 0)
        {
            AdtMessageBox(D_800121CC, it->type, (u32)it->mode);
        }
        it->owner = 0;
        it->proc = 0;
    }
    it++;
    i++;
    goto loop;
end:
    return;
}
