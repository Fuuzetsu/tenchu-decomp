#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "afterimage.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemLaunch(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:3289, 25 src lines, frame 48 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $s3       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s1       struct tag_TItem * item
 *     reg   $s2       struct param_launch * param
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
 *     extern struct ModelType *SyurikenModel;
 * END PSX.SYM */

/*
 * ReqItemLaunch (0x80047738) — spawn a "launch" (catapulted/thrown-and-flying)
 * item. Twin of ReqItemDrop/ReqItemJirai/ReqItemDokudango/ReqItemKaengeki/
 * ReqItemMakibishi/ReqItemLightningBolt/ReqItemNingyo (same item TU, same
 * pool round-robin on COUNTER_FOR_ITEM_ARRAY_ and the same
 * dispose-on-exhaustion block); like ReqItemJirai/ReqItemDokudango there is
 * no GetAreaMapLevel floor check. It gets ProcItemLaunch as its processor.
 *
 * Unlike every other twin, `it->model` is a FIXED global (SyurikenModel), not
 * ItemImage[it->type]. The tail hands the flight to SetupFly (trajectory
 * from p->start to p->end) and then wires up an afterimage trail via
 * SetupAfterimage, stamping its returned struct's two SVECTOR fields and
 * recording the pointer and trailing count in PSX.SYM's `param_launch`.
 *
 * SetupFly's true arity is 6, not Ghidra's 4: Ghidra misses the two
 * STACK-passed trailing args (a common Ghidra limitation past the 4
 * register args) that m2c's raw stack-slot stores catch — the reverse
 * situation from ReqItemMakibishi's SoundEx / ReqItemNingyo's SetNowMotion,
 * where m2c instead over-counted. Always cross-check both against the raw
 * .s when they disagree; here m2c (which showed 6 params) was right.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - Same `cur`/`it` two-pseudo pool search as ReqItemMakibishi/
 *    ReqItemLightningBolt: `cur = items + COUNTER_FOR_ITEM_ARRAY_;` in the
 *    loop/dispose block, `it = cur;` assigned once in the early-exit branch
 *    and once before the dispose block's final owner/proc zeroing (`st`
 *    surviving to the SetupFly call raises register pressure here too).
 *  - `param = &it->param.launch;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `st = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins; unlike most of them, `st` is READ AGAIN as
 *    SetupFly's second argument (same "st survives to a late call" shape as
 *    ReqItemMakibishi's SoundEx(st, ...)).
 *  - us/ty are real temps, same shape as the other twins.
 *  - `it->collision.size = 0; it->model = SyurikenModel;` immediately precede
 *    SetupFly, same position as the other twins' collision-size/model stores
 *    before their own end-vector tail; the scheduler interleaves these
 *    stores with SetupFly's argument setup (independent instructions).
 *  - The `it->param.launch.fly.mode = 0` byte store is written BEFORE the
 *    SetupAfterimage call (textually) — its independence lets the scheduler
 *    drop it into the call's delay slot, same "trailing/preceding
 *    independent store steals the call's delay slot" mechanism as
 *    ReqItemNingyo's `rotate.vx = 0` before its first `rand()`. It MUST go
 *    through a fresh direct `it->param.launch` access, not `param`, even
 *    though it's the same address — same
 *    "field relative to item->param routes through it, not param" lever the other
 *    twins use for `hint = 0`. Getting the base pointer wrong here didn't
 *    change the VALUE stored, only which base register carried it ($s2/param
 *    vs $s1/it), but that's what decided whether this store or the
 *    immediately-following `li $a1, 10` won the delay-slot scheduling tie —
 *    the wrong base left a 9-byte pure-reorder residual (same instructions,
 *    same registers, just this store one slot later) even though the
 *    function was already the right LENGTH.
 *  - `ai = SetupAfterimage(it->model, 10);` is a real temp: the pointer is
 *    stored to `param->effect` AND read six more times for the vector1/
 *    vector2 fields — inlining the call would re-invoke it.
 */
extern void ProcItemLaunch(TItem *item);
extern void SetupFly(param_fly *param, VECTOR *start, VECTOR *end, s32 a4, s32 a5, s32 a6);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;
/* Fixed model for launched items (not indexed by type, unlike the other
 * twins' ItemImage[it->type]). */
extern ModelType *SyurikenModel;

int ReqItemLaunch(PARAM_ITEM_LAUNCH *p)
{
    TItem *it;
    TItem *cur;
    param_launch *param;
    VECTOR *st;
    Humanoid *us;
    s32 ty;
    AfterimageType *ai;
    s32 i;

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
    cur->mode = ITEM_MODE_DISPOSE;
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
    param = &it->param.launch;
    if (it == 0)
        return 0;
    us = p->user;
    ty = p->type;
    it->owner = us;
    it->proc = ProcItemLaunch;
    it->mode = 0;
    it->type = ty;
    it->locate->locate.coord.t[0] = p->start.vx;
    st = &p->start;
    it->locate->locate.coord.t[1] = st->vy;
    it->locate->locate.coord.t[2] = st->vz;
    it->locate->locate.super = 0;
    UpdateCoordinate(it->locate);
    it->collision.size = 0;
    it->model = SyurikenModel;
    SetupFly(&param->fly, st, &p->end, 0x400, 0x400, 0x12c);
    it->param.launch.fly.mode = 0;
    ai = SetupAfterimage(it->model, 10);
    param->effect = ai;
    ai->vector1.vx = 0x14;
    ai->vector1.vy = 0;
    ai->vector1.vz = 0;
    ai->vector2.vx = -0x14;
    ai->vector2.vy = 0;
    ai->vector2.vz = 0;
    param->count = 5;
    return 1;
}
