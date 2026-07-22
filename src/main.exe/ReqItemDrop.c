#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemDrop(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:907, 15 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     reg   $s1       struct param_drop * param
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
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

/*
 * ReqItemDrop (0x8003e8ac) — spawn a dropped/tossed item (called e.g. by
 * ProcItemKusuri when the drink is interrupted). Round-robins through the
 * items[] pool (COUNTER_FOR_ITEM_ARRAY_, this TU's own gp-relative counter);
 * if all 0x1d slots are busy it force-disposes the current one (the same
 * dispose block as the ProcItem* functions). The drop lands only if the area
 * map has floor at the start position; the item then gets ProcItemDrop as its
 * processor and the toss velocity packed into param (param_korogari view).
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `param = &it->param.drop;` sits BEFORE the null check: its addiu
 *    fills the beqz delay slot, and the longer live range demotes param's
 *    allocation priority so p keeps $s1 (after the check, param stole $s1 and the
 *    return-0 block folded away — 2 instructions short, which shifted every
 *    object after this one by 8 bytes).
 *  - us/ty and x/y/z are real temps: the original batches the loads before the
 *    stores (`lw`×3 + `sh`×3) — writing `param->koro.vx = p->end.vx` directly lets the
 *    canonical cc1 emit a truncating `lhu` of the low half instead.
 *  - `st = &p->start;` is materialized between the t[0] and t[1] stores (the
 *    vy/vz reads go through it; vx reads p directly).
 *  - First compiled-to-compiled reference: ProcItemKusuri's `jal ReqItemDrop`
 *    now resolves against this object (R_MIPS_26), and both share item.h.
 */
extern void ProcItemDrop(TItem *item);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;
/* Model pointer per item type. */

int ReqItemDrop(PARAM_ITEM_LAUNCH *p)
{
    TItem *it;
    param_drop *param;
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
    param = &it->param.drop;
    if (it == 0)
        return 0;
    if (GetAreaMapLevel(GlobalAreaMap, p->start.vx, p->start.vy, p->start.vz, 0) < p->start.vy)
        return 0;
    us = p->user;
    ty = p->type;
    it->owner = us;
    it->proc = ProcItemDrop;
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
    it->param.drop.koro.hint = 0;
    param->koro.status = KORO_NORMAL;
    return 1;
}
