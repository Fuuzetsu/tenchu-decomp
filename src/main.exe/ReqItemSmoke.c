#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * ReqItemSmoke (0x80040354) — spawn a thrown smoke-bomb ("smoke") item. Twin
 * of ReqItemDrop/ReqItemJirai/ReqItemDokudango (same item TU, same pool
 * round-robin on COUNTER_FOR_ITEM_ARRAY_ and the same dispose-on-exhaustion
 * block); like ReqItemJirai/ReqItemDokudango there is no GetAreaMapLevel
 * floor check. It gets ProcItemSmoke as its processor, packs the throw
 * velocity into param (param_korogari view, same union member the other
 * twins use), then stamps one extra field past pp->status: offset 0xC, a
 * single BYTE store of 10 — tools/access.py confirms this is a plain `sb`
 * (not the `sw` ReqItemDokudango has at the same offset), so unlike
 * Dokudango's raw offset cast this lines up directly with param_korogari's
 * own `count` field (u8 @ 0xC); `pp->count = 10;` reaches it unchanged.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `pp = (param_korogari *)it->param;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `st = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins.
 *  - us/ty and x/y/z are real temps, same shape as the other twins.
 *  - `((param_korogari *)it->param)->hint = 0;` re-casts it->param (not pp)
 *    for this one store, same as the other twins.
 */
extern void ProcItemSmoke(tag_TItem *item);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;
/* Model pointer per item type. */
extern Sprite3D *D_8008E5BC[];

int ReqItemSmoke(PARAM_ITEM_USE *p)
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
    it->proc = ProcItemSmoke;
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
    pp->count = 10;
    return 1;
}
