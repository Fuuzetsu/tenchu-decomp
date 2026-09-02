#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void UpdateItemState(void);
 *     ITEM.C:3176, 25 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s4       int i
 *     reg   $s1       int mode
 *     reg   $v0       int t
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

extern s32 abs(s32 x);

/* Per-axis item activation distance from ITEM.C's anonymous enum. */
enum
{
    LEN = 15000
};

static void UpdateItemState(void)
{
    ConflictObjectType *conflicts;
    TItem *item;
    GsRVIEW2 *view;
    s32 i;
    s32 hit;
    s32 sz, ofsY;
    ConflictClass mode;
    s16 conflict_id;

    i = 0;
    view = &ViewInfo;
    conflicts = ConflictObject;
    item = items;
loop:
    if (i >= MAX_ITEMS)
        goto done;
    {
        if (item->proc != 0)
        {
            hit = 0;
            if (abs(ViewInfo.vpx - item->locate->locate.coord.t[0]) < LEN &&
                abs(view->vpy - item->locate->locate.coord.t[1]) < LEN)
            {
                hit = abs(view->vpz - item->locate->locate.coord.t[2]) < LEN;
            }
            if (hit)
            {
                if (item->collision.pause != 0)
                {
                    sz = item->collision.size;
                    if (sz != 0)
                    {
                        ofsY = item->collision.ofsY;
                        mode = item->collision.mode;
                        DeleteConflict(item->locate);
                        conflict_id = InsertConflict(item->locate);
                        conflicts[conflict_id].offset.vx = 0;
                        conflicts[conflict_id].offset.vz = 0;
                        conflicts[conflict_id].offset.vy = ofsY;
                        conflicts[conflict_id].size.vz = sz;
                        conflicts[conflict_id].size.vy = sz;
                        conflicts[conflict_id].size.vx = sz;
                        conflicts[conflict_id].common =
                            (void *)CONFLICT_OWNER_ITEM;
                        conflicts[conflict_id].size.pad =
                            mode;
                        item->collision.size = sz;
                        item->collision.ofsY = ofsY;
                        item->collision.mode = mode;
                        item->collision.pause = 0;
                    }
                }
            }
            else if (item->collision.pause == 0 && item->locate->locate.super == 0)
            {
                item->collision.pause = 1;
                DeleteConflict(item->locate);
            }
        }
        item++;
        i++;
        goto loop;
    }
done:;
}
