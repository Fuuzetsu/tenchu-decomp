#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * ReqItemNemuri (0x80045f40) — spawn a thrown sleep-gas ("nemuri") item. Twin
 * of ReqItemDrop/ReqItemJirai/ReqItemSmoke/ReqItemFire/ReqItemDokudango/
 * ReqItemShinsoku (same item TU, same pool round-robin on
 * COUNTER_FOR_ITEM_ARRAY_ and the same dispose-on-exhaustion block); like
 * ReqItemJirai/ReqItemSmoke/ReqItemFire/ReqItemDokudango/ReqItemShinsoku there
 * is no GetAreaMapLevel floor check. It gets ProcItemNemuri as its processor,
 * but differs from the "vanilla" param_korogari twins (Jirai/Smoke/Fire) in
 * two ways (both confirmed against the .s, not just Ghidra):
 *  - `it->model` is unconditionally set from a single fixed global pointer
 *    (Ghidra/the export name it `sprSmoke`, confirmed at 0x80097a68 in
 *    .shake/ghidra-export) — no D_8008E5BC[it->type] lookup by item type.
 *  - the end vector is packed into a DIFFERENT param-union view than
 *    param_korogari: tools/access.py shows three plain sh stores at pp+0,
 *    pp+2, pp+4 (Ghidra's "napalm.vec.vx/vy/vz"), not param_korogari's
 *    vx/vy/vz at +4/+6/+8 — same "distinct union member, reach it via an
 *    offset cast off the SAME proven pointer" situation as
 *    ReqItemDokudango/ReqItemShinsoku's extra stores (see
 *    docs/matching-cookbook.md). No hint/status/count writes at all in this
 *    function (matches ReqItemShinsoku, not the param_korogari twins).
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `pp = (param_korogari *)it->param;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `st = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins.
 *  - us/ty are real temps, same shape as the other twins.
 *  - the end-vector stores are NOT batched through temps here (unlike the
 *    param_korogari twins' x/y/z): the asm interleaves each `lhu` with its
 *    `sh` immediately, so they're written inline through the SAME `pp` cast
 *    (`*(s16 *)((u8 *)pp + N) = p->end.v_;`) — no temp means the truncating
 *    lhu, matching the target exactly (identical shape to ReqItemShinsoku).
 *    The FIRST of the three (+0) compiles through $s0 (it) directly while
 *    the other two (+2/+4) compile through pp's own register ($s2) — a cc1
 *    cse/regalloc artifact, not a source-spelling difference:
 *    ReqItemShinsoku's already-matched source uses the SAME uniform
 *    `pp`-based spelling for all three and produces this exact split.
 *  - unlike ReqItemShinsoku (unconditional return 0), this function returns
 *    1 on success like Jirai/Smoke/Fire — confirmed by the success path's
 *    `li $v0,1` materialized before the tail jump.
 */
extern void ProcItemNemuri(tag_TItem *item);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;
/* Fixed model sprite (Ghidra/export name: sprSmoke); unlike the
 * D_8008E5BC[]-indexing twins this is a single global, not per-type. */
extern Sprite3D *sprSmoke;

int ReqItemNemuri(PARAM_ITEM_USE *p)
{
    tag_TItem *it;
    param_korogari *pp;
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
    pp = (param_korogari *)it->param;
    if (it == 0)
        return 0;
    us = p->user;
    ty = p->type;
    it->owner = us;
    it->proc = ProcItemNemuri;
    it->mode = 0;
    it->type = ty;
    it->locate->locate.coord.t[0] = p->start.vx;
    st = &p->start;
    it->locate->locate.coord.t[1] = st->vy;
    it->locate->locate.coord.t[2] = st->vz;
    it->locate->locate.super = 0;
    UpdateCoordinate(it->locate);
    it->coll_size = 0;
    it->model = sprSmoke;
    *(s16 *)((u8 *)pp + 0) = p->end.vx;
    *(s16 *)((u8 *)pp + 2) = p->end.vy;
    *(s16 *)((u8 *)pp + 4) = p->end.vz;
    return 1;
}
