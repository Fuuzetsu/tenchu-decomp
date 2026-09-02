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

TItem *GetFreeItemSlot(void)
{
    TItem *item;
    s32 i;

    i = 0;
    do
    {
        ic++;
        if (ic >= MAX_ITEMS)
            ic = 0;
        item = items + ic;
        if (item->proc == 0)
            return item;
        i++;
    } while (i < MAX_ITEMS - 1);

    item->mode = ITEM_MODE_DISPOSE;
    item->proc(item);
    DeleteConflict(item->locate);
    if (item->mode != ITEM_MODE_START)
    {
        AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
    }
    item->owner = 0;
    item->proc = 0;
    return item;
}
