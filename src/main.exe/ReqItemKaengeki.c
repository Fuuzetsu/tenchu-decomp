#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemKaengeki(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2344, 20 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     reg   $s1       struct param_kaengeki * param
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
 * ReqItemKaengeki (0x80043c0c) — spawn a "kaengeki" item. Twin of
 * ReqItemDrop/ReqItemJirai/ReqItemSmoke/ReqItemFire/ReqItemDokudango (same
 * item TU, same pool round-robin on ic and the same
 * dispose-on-exhaustion block); like ReqItemJirai/ReqItemSmoke/ReqItemFire/
 * ReqItemDokudango there is no GetAreaMapLevel floor check. It gets
 * ProcItemKaengeki as its processor.
 *
 * This packs p->start and p->end into PSX.SYM's `param_kaengeki` as six full
 * words. start.vx is addressed through `item->param.kaengeki` directly; the
 * other fields use the already-computed `param` pointer.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `param = &item->param.kaengeki;` sits BEFORE the null check, so its
 *    addiu fills the beqz delay slot.
 *  - `pos = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins; dead afterward (p->start.vy/vz are re-read
 *    directly off p, not through pos, in the param tail below).
 *  - aowner/atype are real temps, same shape as the other twins.
 *  - The six start/end fields are INLINE (no x/y/z temps): each compiles to
 *    one lw immediately followed by its sw with no batching, unlike the
 *    s16-narrowing twins which batch three loads before three stores.
 */
extern void ProcItemKaengeki(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemKaengeki(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_kaengeki *param;
    VECTOR *pos;
    Humanoid *aowner;
    s32 atype;
    s32 i;

    i = 0;
    do
    {
        ic++;
        if (0x1d < ic)
            ic = 0;
        item = items + ic;
        if (item->proc == 0)
            goto found;
        i++;
    } while (i < 0x1d);

    /* pool exhausted: force-dispose the slot the counter landed on */
    item->mode = ITEM_MODE_DISPOSE;
    item->proc(item);
    DeleteConflict(item->locate);
    if (item->mode != 0)
    {
        AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
    }
    item->owner = 0;
    item->proc = 0;

found:
    param = &item->param.kaengeki;
    if (item == 0)
        return 0;
    aowner = p->user;
    atype = p->type;
    item->owner = aowner;
    item->proc = ProcItemKaengeki;
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
    item->param.kaengeki.start.vx = p->start.vx;
    param->start.vy = p->start.vy;
    param->start.vz = p->start.vz;
    param->end.vx = p->end.vx;
    param->end.vy = p->end.vy;
    param->end.vz = p->end.vz;
    return 1;
}
