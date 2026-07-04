#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * ReqItemDokudango (0x80041a34) — spawn a thrown poison-dumpling
 * ("dokudango") item. Twin of ReqItemDrop/ReqItemJirai (same item TU, same
 * pool round-robin on COUNTER_FOR_ITEM_ARRAY_ and the same
 * dispose-on-exhaustion block); like ReqItemJirai there is no
 * GetAreaMapLevel floor check. It gets ProcItemDokudango as its processor,
 * packs the throw velocity into param (param_korogari view, same union
 * member ReqItemDrop/ReqItemJirai use), then stamps two extra fields past
 * param_korogari's proven range (offset 0x14 a halfword = 10, offset 0xC a
 * full WORD zero — wider than param_korogari's u8 count at that same offset,
 * so a distinct/overlapping view) before calling SetNowMotion on the user.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `pp = (param_korogari *)it->param;` sits BEFORE the null check, same
 *    lever as ReqItemDrop/ReqItemJirai (addiu fills the beqz delay slot).
 *  - `st = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as ReqItemDrop/ReqItemJirai.
 *  - us/ty and x/y/z are real temps, same shape as ReqItemDrop/ReqItemJirai.
 *  - `((param_korogari *)it->param)->hint = 0;` re-casts it->param (not pp)
 *    for this one store, same as ReqItemDrop/ReqItemJirai.
 *  - The two extra stores past pp->status are raw offset casts off pp
 *    itself (not a fresh it->param cast) — the asm addresses them off pp's
 *    own register, and the word store must stay a full `s32`/`sw` (writing
 *    through a would-be `pp->count` u8 field would wrongly emit `sb`).
 *    Ghidra invents distinct union member paths for these two stores
 *    (`napalm`/`lightningbolt`/`gun`/`smoke.koro` sub-structs) but per the
 *    cookbook's struct-name warning those names aren't trustworthy — only
 *    the raw offsets (proven from the asm) are used here.
 */
extern void ProcItemDokudango(tag_TItem *item);
extern void SetNowMotion(Humanoid *h, s32 mot, s32 loop);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;
/* Model pointer per item type. */
extern Sprite3D *D_8008E5BC[];

int ReqItemDokudango(PARAM_ITEM_USE *p)
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
    it->proc = ProcItemDokudango;
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
    *(s16 *)((u8 *)pp + 0x14) = 10;
    *(s32 *)((u8 *)pp + 0xC) = 0;
    SetNowMotion(it->owner, 0xf02, 1);
    return 1;
}
