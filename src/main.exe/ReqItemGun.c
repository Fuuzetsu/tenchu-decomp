#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * ReqItemGun (0x80046854) — spawn a gun/firearm item. Same item TU, same
 * pool round-robin shape as ReqItemManebue/ReqItemKawarimi/etc, but:
 *  - returns void (no `return 1;`/`return 0;` — Ghidra's decompile shows a
 *    void-returning function; the epilogue never touches $v0)
 *  - instead of `it->model = ItemImage[it->type];`, the tail block-copies
 *    the whole `p->end` VECTOR (4 words, align-4 word block move: 4x lw +
 *    4x sw — see the matching cookbook's "cast type's alignment drives copy
 *    code" rule) onto PSX.SYM's `param_gun.vec`. The raw assembly confirms
 *    plain lw/sw operations for all four words.
 */
extern void ProcItemGun(tag_TItem *item);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;

void ReqItemGun(PARAM_ITEM_LAUNCH *p)
{
    tag_TItem *it;
    VECTOR *st;
    Humanoid *us;
    s32 ty;
    s32 i;

    i = 0;
    do
    {
        COUNTER_FOR_ITEM_ARRAY_++;
        if (0x1d < COUNTER_FOR_ITEM_ARRAY_)
            COUNTER_FOR_ITEM_ARRAY_ = 0;
        it = items + COUNTER_FOR_ITEM_ARRAY_;
        if (it->proc == 0)
            goto found;
        i++;
    } while (i < 0x1d);

    /* pool exhausted: force-dispose the slot the counter landed on */
    it->mode = 0xff;
    it->proc(it);
    DeleteConflict(it->locate);
    if (it->mode != 0)
    {
        AdtMessageBox(D_800121CC, it->type, (u32)it->mode);
    }
    it->owner = 0;
    it->proc = 0;

found:
    if (it == 0)
        return;
    us = p->user;
    ty = p->type;
    it->owner = us;
    it->proc = ProcItemGun;
    it->mode = 0;
    it->type = ty;
    it->locate->locate.coord.t[0] = p->start.vx;
    st = &p->start;
    it->locate->locate.coord.t[1] = st->vy;
    it->locate->locate.coord.t[2] = st->vz;
    it->locate->locate.super = 0;
    UpdateCoordinate(it->locate);
    it->coll_size = 0;
    ((param_gun *)it->param)->vec = p->end;
}
