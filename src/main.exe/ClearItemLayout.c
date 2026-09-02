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

void ClearItemLayout(void)
{
    TItem *it;
    s32 i;

    i = 0;
    it = items;
loop:
    if (i >= MAX_ITEMS)
        return;
    if (it->proc != 0)
    {
        DISPOSE_ITEM(it);
    }
    it++;
    i++;
    goto loop;
}
