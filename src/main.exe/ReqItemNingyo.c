#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemNingyo(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2018, 24 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     reg   $s1       struct param_ningyo * param
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
 * END PSX.SYM */

/*
 * ReqItemNingyo (0x800429f0) — spawn a "ningyo" (doll/puppet decoy?) item.
 * Twin of ReqItemDrop/ReqItemJirai/ReqItemDokudango/ReqItemKaengeki/
 * ReqItemMakibishi/ReqItemLightningBolt (same item TU, same pool round-robin
 * on COUNTER_FOR_ITEM_ARRAY_ and the same dispose-on-exhaustion block); like
 * ReqItemJirai/ReqItemDokudango there is no GetAreaMapLevel floor check. It
 * gets ProcItemNingyo as its processor, packs the end vector into param
 * (param_korogari view, same as ReqItemDrop/ReqItemJirai/ReqItemDokudango),
 * stamps a count=0x5a (matches param_korogari's own `count` field, unlike
 * Dokudango's wider raw-offset store there) plus one more byte at the
 * offset right past param_korogari's proven range (0xD = 0x63, a raw offset
 * cast per the cookbook's divergent-width-union rule), randomizes the
 * model's rotation (rotate.vx=0, rotate.vy = rand()%0x1000, rotate.vz =
 * rand()%0x44), then calls SetNowMotion on the user.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - Unlike ReqItemMakibishi/ReqItemLightningBolt, `it` is NOT split into a
 *    separate loop-cursor pseudo here — it stays in one register ($s0) for
 *    the whole function (lower register pressure at the loop/found-label
 *    join than those two, despite this function's bigger body overall).
 *  - `pp = (param_korogari *)it->param;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `st = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins.
 *  - us/ty and x/y/z (end vector, into pp->vx/vy/vz) are real temps, same
 *    shape as ReqItemDrop/ReqItemJirai/ReqItemDokudango.
 *  - `((param_korogari *)it->param)->hint = 0;` re-casts it->param (not pp)
 *    for this one store, same as the other twins.
 *  - `rotate.vx = 0;` is scheduled into the FIRST `rand()` call's delay slot
 *    (it's independent of the call and textually precedes it — ordinary
 *    scheduler delay-slot filling, no special source shape needed).
 *  - `rotate.vy = rand() % 0x1000;` — power-of-2 modulo, automatic from a
 *    plain `%` (fully compiler-generated sign/shift dance, same idiom family
 *    as the cookbook's magic-multiply rule but the power-of-2 special case).
 *    Its STORE is scheduled into the SECOND `rand()` call's delay slot
 *    (independent, textually precedes it — same mechanism as rotate.vx).
 *  - `rotate.vz = rand() % 0x44;` (0x44 = 68) — non-power-of-2 modulo, the
 *    canonical magic-multiply (0x78787879, shift 5, sign correction).
 *  - SetNowMotion is called with the SAME 3 args as ReqItemDokudango
 *    (`it->owner, 0xf02, 1`) — m2c reports a spurious 4th argument in $a3,
 *    but that register is simply the UNCORRECTED mfhi intermediate from the
 *    `% 0x44` magic-multiply (mfhi->sra 5->subu sign is the real quotient,
 *    kept in a DIFFERENT register and consumed by the `q*68` subtraction;
 *    $a3 itself is never touched again and happens to still hold that value
 *    at the call) — not a real argument. Same m2c-over-counts-args situation
 *    as ReqItemMakibishi's SoundEx call; only 3 args reproduce the bytes.
 *  - `pp->count = 0x5a;` uses param_korogari's own proven `count` field
 *    (offset 0xC) directly — unlike Dokudango's same-offset store, this one
 *    doesn't need a raw-offset cast since the width (u8) and offset match
 *    the established view exactly.
 */
extern void ProcItemNingyo(tag_TItem *item);
extern void SetNowMotion(Humanoid *h, s32 mot, s32 loop);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;
/* Model pointer per item type. */
extern Sprite3D *D_8008E5BC[];

int ReqItemNingyo(PARAM_ITEM_USE *p)
{
    tag_TItem *it;
    param_korogari *pp;
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
    it->proc = ProcItemNingyo;
    it->mode = 0;
    it->type = ty;
    it->locate->locate.coord.t[0] = p->start.vx;
    st = &p->start;
    it->locate->locate.coord.t[1] = st->vy;
    it->locate->locate.coord.t[2] = st->vz;
    it->locate->locate.super = 0;
    UpdateCoordinate(it->locate);
    it->coll_size = 0;
    it->model = D_8008E5BC[it->type];
    x = p->end.vx;
    y = p->end.vy;
    z = p->end.vz;
    pp->vx = x;
    pp->vy = y;
    pp->vz = z;
    ((param_korogari *)it->param)->hint = 0;
    pp->status = 0;
    pp->count = 0x5a;
    it->locate->rotate.vx = 0;
    it->locate->rotate.vy = rand() % 0x1000;
    it->locate->rotate.vz = rand() % 0x44;
    *(u8 *)((u8 *)pp + 0xd) = 0x63;
    SetNowMotion(it->owner, 0xf02, 1);
    return 1;
}
