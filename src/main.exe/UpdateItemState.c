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

/*
 * STATUS: MATCHING (440 bytes).
 *
 * A structured `while` makes loop.c either eliminate `i` in favour of an
 * `items[i]` GIV or bias an explicit cursor to `item + 0x10`.  The literal
 * goto back edge keeps `i` and `item` as independent, unbiased variables.
 * Since that shape also disables loop-invariant hoisting, cache `ViewInfo`
 * and `ConflictObject` explicitly before the label; this reproduces the
 * target's s5/s6 bases without reintroducing a loop GIV. Direct use of the
 * global ConflictObject array shortens the function by two instructions, so
 * the cached array base itself remains source-authored.
 *
 * The first camera coordinate deliberately uses `ViewInfo` directly while
 * the remaining two use `view`, retaining the target's branch-local `lui`
 * plus persistent base. The inserted collision record is then written as one
 * coherent `conflicts[conflict_id]` field graph. CSE forms its element address
 * once in the target operand order; the old integerized element pointer and
 * its `object` alias were compiler artifacts from testing only one access.
 */

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
                        conflicts[conflict_id].offset.components.x = 0;
                        conflicts[conflict_id].offset.components.z = 0;
                        conflicts[conflict_id].offset.components.y = ofsY;
                        conflicts[conflict_id].size.components.z = sz;
                        conflicts[conflict_id].size.components.y = sz;
                        conflicts[conflict_id].size.components.x = sz;
                        conflicts[conflict_id].common.tag = CONFLICT_OWNER_ITEM;
                        conflicts[conflict_id].size.components.class_flags =
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
