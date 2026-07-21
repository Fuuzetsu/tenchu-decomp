#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemGoshikimai(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2262, 21 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s1       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s2       struct param_goshikimai * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

/*
 * ReqItemGoshikimai (0x8004356c) — spawn a thrown "goshikimai" (five-colored
 * rice) item. Twin of ReqItemDrop/ReqItemJirai/ReqItemSmoke/ReqItemFire (same
 * item TU, same pool round-robin on COUNTER_FOR_ITEM_ARRAY_ and the same
 * dispose-on-exhaustion block); like ReqItemJirai/ReqItemSmoke there is no
 * GetAreaMapLevel floor check. It gets ProcItemGoshikimai as its processor.
 * Unlike the twins' rolling-item tail, this stores p->end into PSX.SYM's
 * `param_goshikimai.vec`, the three halfwords at offsets 0/2/4. There is no
 * hint/status/count tail.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `pp = (param_goshikimai *)it->param;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `st = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins.
 *  - us/ty are real temps, same as the other twins.
 *  - The vec.vx store re-casts `it->param` fresh (not `pp`), so its base is
 *    $s0+0x20; vec.vy/vec.vz go through `pp` ($s2).
 *  - All three stores are INLINE (no x/y/z temps): each is a single
 *    lhu-then-sh pair, interleaved in the asm (load, store, load, store,
 *    load, [store in the `return 1;` jump's delay slot]) — not the twins'
 *    batched-loads-then-stores shape.
 */
extern void ProcItemGoshikimai(tag_TItem *item);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;
/* Model pointer per item type. */
extern Sprite3D *ItemImage[];

int ReqItemGoshikimai(PARAM_ITEM_USE *p)
{
    tag_TItem *it;
    param_goshikimai *pp;
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
    pp = (param_goshikimai *)it->param;
    if (it == 0)
        return 0;
    us = p->user;
    ty = p->type;
    it->owner = us;
    it->proc = ProcItemGoshikimai;
    it->mode = 0;
    it->type = ty;
    it->locate->locate.coord.t[0] = p->start.vx;
    st = &p->start;
    it->locate->locate.coord.t[1] = st->vy;
    it->locate->locate.coord.t[2] = st->vz;
    it->locate->locate.super = 0;
    UpdateCoordinate(it->locate);
    it->coll_size = 0;
    it->model = ItemImage[it->type];
    ((param_goshikimai *)it->param)->vec.vx = p->end.vx;
    pp->vec.vy = p->end.vy;
    pp->vec.vz = p->end.vz;
    return 1;
}
