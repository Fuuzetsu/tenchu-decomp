#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PackItemLayout(void *buf, long size);
 *     ITEM.C:473, 30 src lines, frame 224 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       void * buf
 *     param $a1       long size
 *     stack sp+16     unsigned char [200] fn
 *     reg   $a2       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

extern char fmt_item_storing_size_too[]; /* item storing size too small %d/%d */

void PackItemLayout(void *buf, s32 size)
{
    s32 i;
    TItemLayout *slot;
    VECTOR *locate;

    if ((u32)size < sizeof(TItemLayout) * MAX_ITEMS)
    {
        AdtMessageBox(fmt_item_storing_size_too, size,
                      sizeof(TItemLayout) * MAX_ITEMS);
    }
    else
    {
        i = 0;
        do
        {
            slot = &((TItemLayout *)buf)[i];
            if (items[i].proc != 0)
            {
                slot->type = items[i].type;
                slot->locate.vx = items[i].locate->locate.coord.t[0];
                locate = &slot->locate;
                locate->vy = items[i].locate->locate.coord.t[1];
                locate->vz = items[i].locate->locate.coord.t[2];
            }
            else
            {
                slot->type = ITEM_NONE;
            }
            i++;
        } while (i < MAX_ITEMS);
    }
}
