#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemSmoke(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:1463, 18 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     reg   $s1       struct param_smoke * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
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
 * ReqItemSmoke (0x80040354) — spawn a thrown smoke-bomb ("smoke") item. Twin
 * of ReqItemDrop/ReqItemJirai/ReqItemDokudango (same item TU, same pool
 * round-robin on ic and the same dispose-on-exhaustion
 * block); like ReqItemJirai/ReqItemDokudango there is no GetAreaMapLevel
 * floor check. It gets ProcItemSmoke as its processor, packs the throw
 * velocity into the embedded param_smoke.koro record, then initializes the
 * derived type's byte-sized count field to 10.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `param = &item->param.smoke;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `pos = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins.
 *  - aowner/atype and x/y/z are real temps, same shape as the other twins.
 *    The block-scoped `param_korogari *param` is PSX.SYM's second `param`.
 *  - `item->param.smoke.koro.hint = 0;` uses the direct union path (not
 *    `param`) for this one store, same as the other twins.
 */
extern void ProcItemSmoke(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemSmoke(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_smoke *param;
    VECTOR *pos;
    Humanoid *aowner;
    s32 atype;
    s32 x;
    s32 y;
    s32 z;
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
    param = &item->param.smoke;
    if (item == 0)
        return 0;
    aowner = p->user;
    atype = p->type;
    item->owner = aowner;
    item->proc = ProcItemSmoke;
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
    {
        param_korogari *param;

        param = &item->param.smoke.koro;
        x = p->end.vx;
        y = p->end.vy;
        z = p->end.vz;
        param->vx = x;
        param->vy = y;
        param->vz = z;
        item->param.smoke.koro.hint = 0;
        param->status = KORO_NORMAL;
    }
    param->count = 10;
    return 1;
}
