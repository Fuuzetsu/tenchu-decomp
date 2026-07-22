#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemNemuri(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2835, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     reg   $s2       struct param_napalm * param
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
 *     extern struct Sprite3D *sprSmoke;
 * END PSX.SYM */

/*
 * ReqItemNemuri (0x80045f40) — spawn a thrown sleep-gas ("nemuri") item. Twin
 * of ReqItemDrop/ReqItemJirai/ReqItemSmoke/ReqItemFire/ReqItemDokudango/
 * ReqItemShinsoku (same item TU, same pool round-robin on
 * ic and the same dispose-on-exhaustion block); like
 * ReqItemJirai/ReqItemSmoke/ReqItemFire/ReqItemDokudango/ReqItemShinsoku there
 * is no GetAreaMapLevel floor check. It gets ProcItemNemuri as its processor,
 * but differs from the rolling-item twins (Jirai/Smoke/Fire) in
 * two ways (both confirmed against the .s, not just Ghidra):
 *  - `item->model` is unconditionally set from the first shared smoke sprite
 *    (Ghidra/the export names the table `sprSmoke`, confirmed at 0x80097a68 in
 *    .shake/ghidra-export) — no ItemImage[item->type] lookup by item type.
 *  - the end vector is packed into PSX.SYM's `param_napalm.vec` at offsets
 *    0/2/4. No hint/status/count writes occur here.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `param = &item->param.napalm;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `pos = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins.
 *  - aowner/atype are real temps, same shape as the other twins.
 *  - the end-vector stores are NOT batched through temps here (unlike the
 *    rolling-item twins' x/y/z): the asm interleaves each `lhu` with its
 *    `sh` immediately, so they're written inline through `param->vec` — no
 *    temp means the truncating lhu, matching the target exactly (identical
 *    shape to ReqItemShinsoku).
 *    The FIRST of the three (+0) compiles through $s0 (item) directly while
 *    the other two (+2/+4) compile through param's own register ($s2) — a cc1
 *    cse/regalloc artifact, not a source-spelling difference:
 *    ReqItemShinsoku's already-matched source uses the SAME uniform
 *    `param`-based spelling for all three and produces this exact split.
 *  - unlike ReqItemShinsoku (unconditional return 0), this function returns
 *    1 on success like Jirai/Smoke/Fire — confirmed by the success path's
 *    `li $v0,1` materialized before the tail jump.
 */
extern void ProcItemNemuri(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
int ReqItemNemuri(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_napalm *param;
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
    param = &item->param.napalm;
    if (item == 0)
        return 0;
    aowner = p->user;
    atype = p->type;
    item->owner = aowner;
    item->proc = ProcItemNemuri;
    item->mode = 0;
    item->type = atype;
    item->locate->locate.coord.t[0] = p->start.vx;
    pos = &p->start;
    item->locate->locate.coord.t[1] = pos->vy;
    item->locate->locate.coord.t[2] = pos->vz;
    item->locate->locate.super = 0;
    UpdateCoordinate(item->locate);
    item->model = (ModelType *)sprSmoke[0];
    item->collision.size = 0;
    param->vec.vx = p->end.vx;
    param->vec.vy = p->end.vy;
    param->vec.vz = p->end.vz;
    return 1;
}
