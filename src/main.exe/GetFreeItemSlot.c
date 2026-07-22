#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct tag_TItem * GetFreeItemSlot(void);
 *     ITEM.C:577, 20 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 * END PSX.SYM */

/*
 * GetFreeItemSlot (0x8004a42c) — allocate a free slot from items[] via the same
 * round-robin counter as ReqItemDrop (ic): if the slot
 * the counter lands on is free (proc == 0) it's returned immediately;
 * otherwise the counter advances and retries up to 0x1d times, and if the
 * whole pool stays busy the slot the counter last landed on is force-disposed
 * (identical dispose sequence to ReqItemDrop/ProcItemManebue: run its proc
 * with mode set to ITEM_MODE_DISPOSE, delete the conflict, complain if mode
 * didn't clear, then clear owner/proc) and returned anyway.
 *
 * No caller for this function exists anywhere in the retail image (checked:
 * no `jal` and no raw address reference anywhere in main.exe, and none of
 * Ghidra's decompilation of every other function references it either) —
 * dead code kept for byte-identical reproduction. The matching copies in the
 * request functions are consistent with this source helper having been
 * compiler-inlined there while also retaining this out-of-line copy.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - Same do-while shape as ReqItemDrop (`i = 0;` first, since there's no
 *    parameter to cache ahead of it here), but the early exit is
 *    `if (item->proc == 0) return item;` rather than `goto found;`: with no
 *    drop-specific code after the loop, cc1 emits the return-value move in
 *    the branch's own delay slot instead of falling into shared code.
 */

TItem *GetFreeItemSlot(void)
{
    TItem *item;
    s32 i;

    i = 0;
    do
    {
        ic++;
        if (0x1d < ic)
            ic = 0;
        item = items + ic;
        if (item->proc == 0)
            return item;
        i++;
    } while (i < 0x1d);

    item->mode = ITEM_MODE_DISPOSE;
    item->proc(item);
    DeleteConflict(item->locate);
    if (item->mode != 0)
    {
        AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
    }
    item->owner = 0;
    item->proc = 0;
    return item;
}
