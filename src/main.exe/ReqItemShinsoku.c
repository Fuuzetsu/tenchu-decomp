#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemShinsoku(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:1383, 13 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     reg   $s2       struct param_shinsoku * param
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
 * END PSX.SYM */

/*
 * ReqItemShinsoku (0x8003fde0) — spawn a "shinsoku" (instant-movement /
 * fire?) item. Twin of ReqItemDrop/ReqItemJirai/ReqItemSmoke/ReqItemFire/
 * ReqItemDokudango (same item TU, same pool round-robin on
 * COUNTER_FOR_ITEM_ARRAY_ and the same dispose-on-exhaustion block); like
 * ReqItemJirai/ReqItemSmoke/ReqItemFire/ReqItemDokudango there is no
 * GetAreaMapLevel floor check. It gets ProcItemShinsoku as its processor, but
 * differs from every other twin in three ways (all confirmed against the
 * .s, not just Ghidra):
 *  - `it->model` is unconditionally zeroed — no ItemImage[it->type] lookup.
 *  - the end vector is packed into a DIFFERENT param-union view than
 *    param_korogari: tools/access.py shows three PLAIN sh stores at pp+0,
 *    pp+2, pp+4 (Ghidra's "napalm.vec.vx/vy/vz"), not param_korogari's
 *    vx/vy/vz at +4/+6/+8 — same "distinct union member, reach it via an
 *    offset cast off the SAME proven pointer" situation as
 *    ReqItemDokudango's two extra stores (see docs/matching-cookbook.md).
 *    No hint/status/count writes at all in this function.
 *  - both the "pool exhausted" early return and the normal path return 0
 *    (confirmed: both epilogue predecessors set $v0 via `addu $v0,$zero,$zero`)
 *    — unlike the other twins, which return 1 on the normal path.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `pp = (param_korogari *)it->param;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `st = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins.
 *  - us/ty are real temps, same shape as the other twins.
 *  - the end-vector stores are NOT batched through temps here (unlike the
 *    other twins' x/y/z): the asm interleaves each `lhu` with its `sh`
 *    immediately, so they're written inline (`*(s16 *)(...) = p->end.vx;`)
 *    per the cookbook's "narrowing store fed through a temp forces the
 *    full-word load" rule — no temp here means the truncating lhu, matching
 *    the target exactly.
 *  - there is no `((param_korogari *)it->param)->hint = 0;` store in this
 *    function (no hint/status/count touched at all).
 */
extern void ProcItemShinsoku(tag_TItem *item);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;

int ReqItemShinsoku(PARAM_ITEM_USE *p)
{
    tag_TItem *it;
    param_korogari *pp;
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
    pp = (param_korogari *)it->param;
    if (it == 0)
        return 0;
    us = p->user;
    ty = p->type;
    it->owner = us;
    it->proc = ProcItemShinsoku;
    it->mode = 0;
    it->type = ty;
    it->locate->locate.coord.t[0] = p->start.vx;
    st = &p->start;
    it->locate->locate.coord.t[1] = st->vy;
    it->locate->locate.coord.t[2] = st->vz;
    it->locate->locate.super = 0;
    UpdateCoordinate(it->locate);
    it->coll_size = 0;
    it->model = 0;
    *(s16 *)((u8 *)pp + 0) = p->end.vx;
    *(s16 *)((u8 *)pp + 2) = p->end.vy;
    *(s16 *)((u8 *)pp + 4) = p->end.vz;
    return 0;
}
