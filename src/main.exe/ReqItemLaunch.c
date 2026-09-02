#include "common.h"
#include "tuning.h"
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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * pool round-robin on ic and the same
 * dispose-on-exhaustion block); like ReqItemJirai/ReqItemDokudango there is
 * no GetAreaMapLevel floor check. It gets ProcItemLaunch as its processor.
 *
 * Unlike every other twin, `item->model` is a FIXED global (SyurikenModel), not
 * ItemImage[item->type]. The tail hands the flight to SetupFly (trajectory
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
 *  - The inlined allocator keeps PSX.SYM's `ret` separate from the outer
 *    `item`, as in ReqItemMakibishi/ReqItemLightningBolt:
 *    `ret = items + ic;` in the loop/dispose block, with `item = ret;`
 *    once in the early-exit branch and once before owner/proc zeroing (`pos`
 *    surviving to the SetupFly call raises register pressure here too).
 *  - `param = &item->param.launch;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `pos = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins; unlike most of them, `pos` is READ AGAIN as
 *    SetupFly's second argument (same "pos survives to a late call" shape as
 *    ReqItemMakibishi's SoundEx(pos, ...)).
 *  - aowner/atype are real temps, same shape as the other twins.
 *  - `item->collision.size = 0; item->model = SyurikenModel;` immediately precede
 *    SetupFly, same position as the other twins' collision-size/model stores
 *    before their own end-vector tail; the scheduler interleaves these
 *    stores with SetupFly's argument setup (independent instructions).
 *  - The `item->param.launch.fly.mode = FLY_MODE_ARC` byte store is written BEFORE the
 *    SetupAfterimage call (textually) — its independence lets the scheduler
 *    drop it into the call's delay slot, same "trailing/preceding
 *    independent store steals the call's delay slot" mechanism as
 *    ReqItemNingyo's `rotate.vx = 0` before its first `rand()`. It MUST go
 *    through a fresh direct `item->param.launch` access, not `param`, even
 *    though it's the same address — same
 *    "field relative to item->param routes through item, not param" lever the other
 *    twins use for `hint = 0`. Getting the base pointer wrong here didn't
 *    change the VALUE stored, only which base register carried it ($s2/param
 *    vs $s1/item), but that's what decided whether this store or the
 *    immediately-following `li $a1, 10` won the delay-slot scheduling tie —
 *    the wrong base left a 9-byte pure-reorder residual (same instructions,
 *    same registers, just this store one slot later) even though the
 *    function was already the right LENGTH.
 *  - `ai = SetupAfterimage(item->model, 10);` is a real temp: the pointer is
 *    stored to `param->effect` AND read six more times for the vector1/
 *    vector2 fields — inlining the call would re-invoke it.
 */
extern void ProcItemLaunch(TItem *item);
extern void SetupFly(param_fly *param, VECTOR *start, VECTOR *end, s32 a4, s32 a5, s32 a6);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemLaunch(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    TItem *ret;
    param_launch *param;
    VECTOR *pos;
    AfterimageType *ai;
    s32 i;

    TAKE_ITEM_SLOT_VIA_CURSOR();
    param = &item->param.launch;
    if (item == 0)
        return 0;
    {
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemLaunch);
        item->collision.size = 0;
        item->model = SyurikenModel;
    }
    SetupFly(&param->fly, pos, &p->end, FIXED_QUARTER, FIXED_QUARTER, 300);
    item->param.launch.fly.mode = FLY_MODE_ARC;
    ai = SetupAfterimage(item->model, 10);
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
