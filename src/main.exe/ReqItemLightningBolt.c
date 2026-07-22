#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemLightningBolt(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2917, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_lightningbolt * param
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
 * ReqItemLightningBolt (0x80046358) — spawn a "lightning bolt" item. Twin of
 * ReqItemDrop/ReqItemJirai/ReqItemDokudango/ReqItemKaengeki/ReqItemMakibishi
 * (same item TU, same pool round-robin on COUNTER_FOR_ITEM_ARRAY_ and the
 * same dispose-on-exhaustion block); like ReqItemJirai/ReqItemKaengeki there
 * is no GetAreaMapLevel floor check. It gets ProcItemLightningBolt as its
 * processor.
 *
 * PSX.SYM identifies the payload as `param_lightningbolt`: start is stored
 * as three full words, with start.vx through `it->param.lightningbolt`
 * directly and start.vy/start.vz through `pp`. There is no end-vector store — instead
 * GetVectorRotation(&p->start, &p->end, &rot.vx, &rot.vz) computes a rotation
 * from start/end into a LOCAL SVECTOR, of which only .vx/.vz are read back
 * (skipping .vy — the two out-arg stack slots are 4 bytes apart, matching
 * SVECTOR's vx@0/vz@4 with vy@2 skipped, not 2 independent u16s); the two
 * results become `param_lightningbolt.rot.vx/.vy`, with `.vz` cleared.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - Same `cur`/`it` two-pseudo pool search as ReqItemMakibishi: `cur = items
 *    + COUNTER_FOR_ITEM_ARRAY_;` in the loop/dispose block, `it = cur;`
 *    assigned once in the early-exit branch and once before the dispose
 *    block's final owner/proc zeroing — this function's register pressure
 *    (SVECTOR local + pp + it + p all live around the tail) again pushes
 *    `cur`/`it` to different hard registers, making the transfer a real
 *    `move` (see the cookbook rule this pair of functions taught).
 *  - `pp = &it->param.lightningbolt;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `st = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins; dead afterward (p->start.vy/vz are re-read
 *    directly off p, not through st, in the param tail below, same as
 *    ReqItemKaengeki).
 *  - us/ty are real temps, same shape as the other twins.
 *  - The three start-vector param stores are INLINE (no x/y/z temps): each
 *    compiles to one lw immediately followed by its sw, same as
 *    ReqItemKaengeki's six-word tail.
 *  - `rot.vx`/`rot.vz` ARE read into separate temps (rx, ry) BEFORE any of
 *    the three rot.vx/rot.vy/rot.vz stores (batched loads-before-stores,
 *    same shape as the other twins' x/y/z end-vector temps) — reading them
 *    inline at each store point interleaves a store between the two loads
 *    and costs 4 bytes plus a different schedule.
 */
extern void ProcItemLightningBolt(TItem *item);
extern void GetVectorRotation(VECTOR *from, VECTOR *to, short *out1, short *out2);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;
/* Model pointer per item type. */
extern Sprite3D *ItemImage[];

int ReqItemLightningBolt(PARAM_ITEM_LAUNCH *p)
{
    TItem *it;
    TItem *cur;
    param_lightningbolt *pp;
    VECTOR *st;
    Humanoid *us;
    s32 ty;
    SVECTOR rot;
    s16 rx;
    s16 ry;
    s32 i;

    i = 0;
    do
    {
        COUNTER_FOR_ITEM_ARRAY_++;
        if (0x1d < COUNTER_FOR_ITEM_ARRAY_)
            COUNTER_FOR_ITEM_ARRAY_ = 0;
        cur = items + COUNTER_FOR_ITEM_ARRAY_;
        if (cur->proc == 0)
        {
            it = cur;
            goto found;
        }
        i++;
    } while (i < 0x1d);

    /* pool exhausted: force-dispose the slot the counter landed on */
    cur->mode = 0xff;
    cur->proc(cur);
    DeleteConflict(cur->locate);
    if (cur->mode != 0)
    {
        AdtMessageBox(D_800121CC, cur->type, (u32)cur->mode);
    }
    it = cur;
    it->owner = 0;
    it->proc = 0;

found:
    pp = &it->param.lightningbolt;
    if (it == 0)
        return 0;
    us = p->user;
    ty = p->type;
    it->owner = us;
    it->proc = ProcItemLightningBolt;
    it->mode = 0;
    it->type = ty;
    it->locate->locate.coord.t[0] = p->start.vx;
    st = &p->start;
    it->locate->locate.coord.t[1] = st->vy;
    it->locate->locate.coord.t[2] = st->vz;
    it->locate->locate.super = 0;
    UpdateCoordinate(it->locate);
    it->collision.size = 0;
    it->model = (ModelType *)ItemImage[it->type];
    it->param.lightningbolt.start.vx = p->start.vx;
    pp->start.vy = p->start.vy;
    pp->start.vz = p->start.vz;
    GetVectorRotation(&p->start, &p->end, &rot.vx, &rot.vz);
    rx = rot.vx;
    ry = rot.vz;
    pp->rot.vz = 0;
    pp->rot.vx = rx;
    pp->rot.vy = ry;
    return 1;
}
