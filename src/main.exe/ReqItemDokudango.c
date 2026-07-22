#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemDokudango(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:1777, 23 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     reg   $s1       struct param_dokudango * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *     reg   $s1       struct param_korogari * param
 *     reg   $v0       int x
 *     reg   $v1       int y
 *     reg   $a0       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

/*
 * ReqItemDokudango (0x80041a34) — spawn a thrown poison-dumpling
 * ("dokudango") item. Twin of ReqItemDrop/ReqItemJirai (same item TU, same
 * pool round-robin on ic and the same
 * dispose-on-exhaustion block); like ReqItemJirai there is no
 * GetAreaMapLevel floor check. It gets ProcItemDokudango as its processor,
 * packs the throw velocity into the embedded param_dokudango.koro record,
 * clears eater, initializes count, then calls SetNowMotion on the user.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `param = &it->param.dokudango;` sits BEFORE the null check, same
 *    lever as ReqItemDrop/ReqItemJirai (addiu fills the beqz delay slot).
 *  - `st = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as ReqItemDrop/ReqItemJirai.
 *  - us/ty and x/y/z are real temps, same shape as ReqItemDrop/ReqItemJirai.
 *  - `it->param.dokudango.koro.hint = 0;` uses the direct union path (not
 *    `param`) for this one store, same as ReqItemDrop/ReqItemJirai.
 *  - PSX.SYM supplies the koro/eater/org_think/count nesting and names. The
 *    retail executable accesses count as a halfword, widened from the demo's
 *    byte, so the retail declaration retains that one version difference.
 */
extern void ProcItemDokudango(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */

int ReqItemDokudango(PARAM_ITEM_LAUNCH *p)
{
    TItem *it;
    param_dokudango *param;
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
        ic++;
        if (0x1d < ic)
            ic = 0;
        it = items + ic;
        if (it->proc == 0)
            goto found;
        i++;
    } while (i < 0x1d);

    /* pool exhausted: force-dispose the slot the counter landed on */
    it->mode = ITEM_MODE_DISPOSE;
    it->proc(it);
    DeleteConflict(it->locate);
    if (it->mode != 0)
    {
        AdtMessageBox(D_800121CC, it->type, (u32)it->mode);
    }
    it->owner = 0;
    it->proc = 0;

found:
    param = &it->param.dokudango;
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
    it->collision.size = 0;
    it->model = (ModelType *)ItemImage[it->type];
    x = p->end.vx;
    y = p->end.vy;
    z = p->end.vz;
    param->koro.vx = x;
    param->koro.vy = y;
    param->koro.vz = z;
    it->param.dokudango.koro.hint = 0;
    param->koro.status = KORO_NORMAL;
    param->count = 10;
    param->eater = 0;
    SetNowMotion(it->owner, 0xf02, 1);
    return 1;
}
