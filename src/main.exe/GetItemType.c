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

TItemType GetItemType(s32 ConflictID)
{
    s32 i;

    for (i = 0; i < MAX_ITEMS; i++)
    {
        if (items[i].locate->id == ConflictID)
        {
            return items[i].type;
        }
    }
    return ITEM_KAGINAWA;
}
