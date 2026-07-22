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
 * (same item TU, same pool round-robin on ic and the
 * same dispose-on-exhaustion block); like ReqItemJirai/ReqItemKaengeki there
 * is no GetAreaMapLevel floor check. It gets ProcItemLightningBolt as its
 * processor.
 *
 * PSX.SYM identifies the payload as `param_lightningbolt`: start is stored
 * as three full words, with start.vx through `item->param.lightningbolt`
 * directly and start.vy/start.vz through `param`. There is no end-vector store — instead
 * GetVectorRotation(&p->start, &p->end, &rx, &ry) computes two full-word
 * rotation outputs; their low halves become `param_lightningbolt.rot.vx/.vy`,
 * with `.vz` cleared.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - The inlined allocator keeps `slot` separate from PSX.SYM's outer
 *    `item`, as in ReqItemMakibishi: `slot = items + ic;` in the
 *    loop/dispose block, with `item = slot;`
 *    assigned once in the early-exit branch and once before the dispose
 *    block's final owner/proc zeroing — this function's register pressure
 *    (stack rotation outputs + param + item + p all live around the tail)
 *    pushes `slot`/`item` to different hard registers, making the transfer a real
 *    `move` (see the cookbook rule this pair of functions taught).
 *  - `param = &item->param.lightningbolt;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `pos = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins; dead afterward (p->start.vy/vz are re-read
 *    directly off p, not through pos, in the param tail below, same as
 *    ReqItemKaengeki).
 *  - aowner/atype are real temps, same shape as the other twins.
 *  - The three start-vector param stores are INLINE (no x/y/z temps): each
 *    compiles to one lw immediately followed by its sw, same as
 *    ReqItemKaengeki's six-word tail.
 *  - `rx`/`ry` are read before any of the three result stores (batched
 *    loads-before-stores, the same shape as the other twins' x/y/z temps).
 */
extern void ProcItemLightningBolt(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemLightningBolt(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    TItem *slot;
    param_lightningbolt *param;
    VECTOR *pos;
    Humanoid *aowner;
    s32 atype;
    int rx;
    int ry;
    s32 i;

    i = 0;
    do
    {
        ic++;
        if (0x1d < ic)
            ic = 0;
        slot = items + ic;
        if (slot->proc == 0)
        {
            item = slot;
            goto found;
        }
        i++;
    } while (i < 0x1d);

    /* pool exhausted: force-dispose the slot the counter landed on */
    slot->mode = ITEM_MODE_DISPOSE;
    slot->proc(slot);
    DeleteConflict(slot->locate);
    if (slot->mode != 0)
    {
        AdtMessageBox(D_800121CC, slot->type, (u32)slot->mode);
    }
    item = slot;
    item->owner = 0;
    item->proc = 0;

found:
    param = &item->param.lightningbolt;
    if (item == 0)
        return 0;
    aowner = p->user;
    atype = p->type;
    item->owner = aowner;
    item->proc = ProcItemLightningBolt;
    item->mode = 0;
    item->type = atype;
    item->locate->locate.coord.t[0] = p->start.vx;
    pos = &p->start;
    item->locate->locate.coord.t[1] = pos->vy;
    item->locate->locate.coord.t[2] = pos->vz;
    item->locate->locate.super = 0;
    UpdateCoordinate(item->locate);
    item->collision.size = 0;
    item->model = (ModelType *)ItemImage[item->type];
    item->param.lightningbolt.start.vx = p->start.vx;
    param->start.vy = p->start.vy;
    param->start.vz = p->start.vz;
    GetVectorRotation(&p->start, &p->end, &rx, &ry);
    param->rot.vz = 0;
    param->rot.vx = rx;
    param->rot.vy = ry;
    return 1;
}
