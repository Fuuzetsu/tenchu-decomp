#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * ReqItemJirai (0x80048958) — spawn a placed landmine/trap item ("jirai").
 * Twin of ReqItemDrop (same item TU, same pool round-robin on
 * COUNTER_FOR_ITEM_ARRAY_ and the same dispose-on-exhaustion block); unlike
 * ReqItemDrop there is no GetAreaMapLevel floor check — a jirai is placed
 * unconditionally. It gets ProcItemJirai as its processor and the trigger
 * velocity packed into param (param_korogari view, same union member
 * ReqItemDrop uses).
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `pp = (param_korogari *)it->param;` sits BEFORE the null check, same
 *    lever as ReqItemDrop (addiu fills the beqz delay slot).
 *  - `st = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as ReqItemDrop (vy/vz reads go through it; vx reads p directly).
 *  - us/ty are real temps, same as ReqItemDrop: the asm loads both p->user
 *    and p->type back-to-back before any of the owner/proc/mode/type stores.
 *  - x/y/z (end vector) ARE real temps: the asm batches three loads before
 *    three sh stores, matching ReqItemDrop's pp->vx/vy/vz shape exactly.
 *  - `((param_korogari *)it->param)->hint = 0;` re-casts it->param (not pp)
 *    for this one store, same as ReqItemDrop.
 */
extern void ProcItemJirai(tag_TItem *item);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;
/* Model pointer per item type. */
extern Sprite3D *D_8008E5BC[];

int ReqItemJirai(PARAM_ITEM_USE *p)
{
    tag_TItem *it;
    param_korogari *pp;
    VECTOR *st;
    Humanoid *us;
    s32 ty;
    s32 x;
    s32 y;
    s32 z;
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
    pp = (param_korogari *)it->param;
    if (it == 0)
        return 0;
    us = p->user;
    ty = p->type;
    it->owner = us;
    it->proc = ProcItemJirai;
    it->mode = 0;
    it->type = ty;
    it->locate->locate.coord.t[0] = p->start.vx;
    st = &p->start;
    it->locate->locate.coord.t[1] = st->vy;
    it->locate->locate.coord.t[2] = st->vz;
    it->locate->locate.super = 0;
    UpdateCoordinate(it->locate);
    it->coll_size = 0;
    it->model = D_8008E5BC[it->type];
    x = p->end.vx;
    y = p->end.vy;
    z = p->end.vz;
    pp->vx = x;
    pp->vy = y;
    pp->vz = z;
    ((param_korogari *)it->param)->hint = 0;
    pp->status = 0;
    return 1;
}
