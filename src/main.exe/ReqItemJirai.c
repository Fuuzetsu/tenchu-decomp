#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemJirai(struct PARAM_ITEM_DROP *p);
 *     ITEM.C:3609, 18 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s2       struct PARAM_ITEM_DROP * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_smoke * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       int atype
 *     reg   $a0       struct VECTOR * pos
 *     reg   $s1       struct param_korogari * param
 *     reg   $v1       int x
 *     reg   $a0       int y
 *     reg   $a1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

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
 *  - `param = &it->param.smoke;` sits BEFORE the null check, same
 *    lever as ReqItemDrop (addiu fills the beqz delay slot).
 *  - `st = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as ReqItemDrop (vy/vz reads go through it; vx reads p directly).
 *  - us/ty are real temps, same as ReqItemDrop: the asm loads both p->user
 *    and p->type back-to-back before any of the owner/proc/mode/type stores.
 *  - x/y/z (end vector) ARE real temps: the asm batches three loads before
 *    three sh stores, matching ReqItemDrop's koro.vx/vy/vz shape exactly.
 *  - `it->param.smoke.koro.hint = 0;` uses the direct union path (not
 *    `param`) for this one store, same as ReqItemDrop.
 */
extern void ProcItemJirai(tag_TItem *item);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;
/* Model pointer per item type. */
extern Sprite3D *ItemImage[];

int ReqItemJirai(PARAM_ITEM_LAUNCH *p)
{
    tag_TItem *it;
    param_smoke *param;
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
    param = &it->param.smoke;
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
    it->model = (ModelType *)ItemImage[it->type];
    x = p->end.vx;
    y = p->end.vy;
    z = p->end.vz;
    param->koro.vx = x;
    param->koro.vy = y;
    param->koro.vz = z;
    it->param.smoke.koro.hint = 0;
    param->koro.status = 0;
    return 1;
}
