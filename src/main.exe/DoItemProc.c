#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DoItemProc(void);
 *     ITEM.C:3205, 52 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

extern void InitializeItem(void);
extern void UpdateItemState(void);

void DoItemProc(void)
{
    TItem *it;
    s32 i;

    if (Item_fInitial == 0)
    {
        InitializeItem();
    }
    if (GameClock % 10 == 0)
    {
        UpdateItemState();
    }
    i = 0;
    it = items;
    while (1)
    {
        if (i >= MAX_ITEMS)
            break;
        if (it->proc != 0)
        {
            it->proc(it);
        }
        it++;
        i++;
    }
}
