#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemArrow(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:3488, 16 src lines, frame 48 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     reg   $s1       struct tag_TItem * item
 *     reg   $s3       struct param_arrow * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $s0       struct VECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * ReqItemArrow (0x800480c4) — spawn a homing/auto-aim arrow. Twin of
 * ReqItemDrop/ReqItemJirai/ReqItemDokudango/ReqItemLaunch (same item TU, same
 * pool round-robin on COUNTER_FOR_ITEM_ARRAY_ and the same
 * dispose-on-exhaustion block); like ReqItemLaunch, `it->model` is a FIXED
 * global (D_80097F4C, gp-relative in this TU like ReqItemLaunch's
 * D_80097F48) rather than D_8008E5BC[it->type], and the tail hands off to
 * SetupFly. Unlike every other twin, there's a pre-pool-search step: it
 * computes the aim direction from p->start/p->end (GetVectorRotation), packs
 * it into an SVECTOR, and feeds that to SearchItemTarget2 to find an actual
 * target position (`target`) BEFORE the round-robin allocation even starts —
 * that result becomes SetupFly's "end" vector instead of p->end directly.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `&p->start`/`&p->end` are passed INLINE to GetVectorRotation/
 *    SearchItemTarget2 (no shared `st` temp for these two calls) — `st` only
 *    exists as the twins' usual tail-only temp (`st = &p->start;` right
 *    before the coord.t[1]/[2] reads, same statement position as every
 *    twin). Caching `&p->start` in a named variable spanning BOTH the early
 *    calls and the tail forced it into a different callee-saved register
 *    than the pool cursor/`it`, swapping $s0/$s1 vs the twins' assignment —
 *    inlining the two early address-of expressions (still CSE'd by the
 *    compiler within that straight-line prefix) reproduced the twins' exact
 *    allocation.
 *  - **Ghidra's two `short local_20[2]`/`short local_1c[2]` out-params for
 *    GetVectorRotation must be ONE combined struct/array, not two separate
 *    same-shaped locals** — declaring them as independent stack objects
 *    (whether scalar `u16`, `u16[2]`, or a padded struct) triggers EXTRA
 *    padding between them that doesn't belong (each attempt either packed
 *    them 2 bytes too tight or, once either was sized/aligned to 4 bytes,
 *    overshot by a further 4 bytes — never landing on the target's exact
 *    4-byte spacing). Declaring one `struct { u16 rx,pad0,ry,pad1; } rot;`
 *    and using `&rot.rx`/`&rot.ry` matched immediately: the pair is a single
 *    8-byte aggregate the original stack-shared for BOTH out-params, only
 *    the low half of each 4-byte slot actually read back (`lhu`, matching
 *    Ghidra's `(int *)` casts on the call — the callee likely writes a full
 *    word, the caller only needs the low 16 bits).
 *  - Same `cur`/`it` two-pseudo pool search as ReqItemLaunch (confirmed
 *    against the raw .s, not inferred): `cur = items + COUNTER...;` used
 *    throughout the loop/dispose block, `it = cur;` assigned exactly twice
 *    (the early-exit branch and right before the dispose block falls into
 *    `found:`) — the heavier tail (p/st/pp all needing registers across
 *    SetupFly) raises pressure the same way ReqItemLaunch's does.
 *  - `pp = it->param;` (plain `u8 *`, no param_korogari cast — no named
 *    field of that view is read/written here) sits BEFORE the null check,
 *    same delay-slot lever as every twin; reused unchanged for both
 *    SetupFly's 1st arg and the final `pp[0x2c] = 5;` store (unlike
 *    ReqItemLaunch's analogous store, which needed a FRESH it->param cast
 *    for its own scheduling tie — here the cached pp reproduces the target
 *    directly).
 *  - `us`/`ty` temps for owner/type, same shape as the other twins (loaded
 *    back-to-back, stored owner/proc/mode/type in that order).
 *  - GetVectorRotation's out-params are read back with `lhu` (unsigned
 *    halfword), so `rot.rx`/`rot.ry` are `u16`, not ReqItemLightningBolt's
 *    TU's `short *` — same "respell the callee's prototype per-TU" situation
 *    as ReqItemDefault's GetVectorRotation call.
 *  - `it->coll_size = 0; it->model = D_80097F4C;` immediately precede
 *    SetupFly, same position/interleaving as ReqItemLaunch.
 */
extern void ProcItemArrow(tag_TItem *item);
extern void GetVectorRotation(VECTOR *from, VECTOR *to, u16 *out1, u16 *out2);
extern void SearchItemTarget2(Humanoid *user, SVECTOR *dir, VECTOR *from, VECTOR *target);
extern void SetupFly(void *param, VECTOR *start, VECTOR *end, s32 a4, s32 a5, s32 a6);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;
/* Fixed model for arrows (not indexed by type, unlike the pool-drop twins'
 * D_8008E5BC[it->type]); gp-relative in this TU like ReqItemLaunch's
 * D_80097F48. */
extern Sprite3D *D_80097F4C;

int ReqItemArrow(PARAM_ITEM_USE *p)
{
    tag_TItem *it;
    tag_TItem *cur;
    u8 *pp;
    VECTOR *st;
    Humanoid *us;
    s32 ty;
    SVECTOR dir;
    VECTOR target;
    struct
    {
        u16 rx;
        u16 pad0;
        u16 ry;
        u16 pad1;
    } rot;
    s32 i;

    GetVectorRotation(&p->start, &p->end, &rot.rx, &rot.ry);
    dir.vz = 0;
    dir.vx = rot.rx;
    dir.vy = rot.ry;
    SearchItemTarget2(p->user, &dir, &p->start, &target);
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
    pp = it->param;
    if (it == 0)
        return 0;
    us = p->user;
    ty = p->type;
    it->owner = us;
    it->proc = ProcItemArrow;
    it->mode = 0;
    it->type = ty;
    it->locate->locate.coord.t[0] = p->start.vx;
    st = &p->start;
    it->locate->locate.coord.t[1] = st->vy;
    it->locate->locate.coord.t[2] = st->vz;
    it->locate->locate.super = 0;
    UpdateCoordinate(it->locate);
    it->coll_size = 0;
    it->model = D_80097F4C;
    SetupFly(pp, st, &target, 0, 0x800, 0x12c);
    pp[0x2c] = 5;
    return 1;
}
