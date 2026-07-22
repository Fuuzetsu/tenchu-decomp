#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemKusuri(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:1549, 18 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

/*
 * ReqItemKusuri (0x80040a98) — spawn a "kusuri" (medicine/health potion)
 * item. Same item-TU pool round-robin on ic and the same
 * dispose-on-exhaustion block as ReqItemJirai/ReqItemShinsoku; unlike those
 * twins, kusuri is a plain "use" item with no rolling/placement physics: it
 * never touches it->param at all (no param_korogari view, no end-vector
 * store, no hint/status/count) — confirmed by tools/access.py, whose last
 * body access is the ItemImage[it->type] model load, immediately followed
 * by the epilogue register reloads. It gets ProcItemKusuri as its processor
 * and returns 1 on the normal path (like Jirai; Shinsoku is the outlier that
 * returns 0 on both paths).
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `st = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as Jirai/Shinsoku.
 *  - us/ty are real temps, same shape as the other twins.
 *  - `it->locate` is NOT cached into a pointer temp: it is reloaded fresh at
 *    every one of its 5 textual uses (t[0]/t[1]/t[2]/super/UpdateCoordinate
 *    argument) — access.py shows 5 separate `lw $s0,0x10` reloads. The very
 *    first of those reloads is scheduled by cc1 BEFORE the proc/mode/type
 *    stores even though it textually follows them in source (independent
 *    loads get hoisted ahead of independent stores); write the natural
 *    owner/proc/mode/type/coord order and let the scheduler do this, exactly
 *    like Jirai.
 *  - no `pp`/param_korogari view at all — this function has nothing to do
 *    with it->param.
 */
extern void ProcItemKusuri(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */

int ReqItemKusuri(PARAM_ITEM_LAUNCH *p)
{
    TItem *it;
    VECTOR *st;
    Humanoid *us;
    s32 ty;
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
    if (it == 0)
        return 0;
    us = p->user;
    ty = p->type;
    it->owner = us;
    it->proc = ProcItemKusuri;
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
    return 1;
}
