#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemArrow(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:3488, 16 src lines, frame 48 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 *     extern struct ModelType *ArrowModel;
 * END PSX.SYM */

/*
 * ReqItemArrow (0x800480c4) — spawn a homing/auto-aim arrow. Twin of
 * ReqItemDrop/ReqItemJirai/ReqItemDokudango/ReqItemLaunch (same item TU, same
 * pool round-robin on ic and the same
 * dispose-on-exhaustion block); like ReqItemLaunch, `item->model` is a FIXED
 * global (ArrowModel, gp-relative in this TU like ReqItemLaunch's
 * SyurikenModel) rather than ItemImage[item->type], and the tail hands off to
 * SetupFly. Unlike every other twin, there's a pre-pool-search step: it
 * computes the aim direction from p->start/p->end (GetVectorRotation), packs
 * it into an SVECTOR, and feeds that to SearchItemTarget2 to find an actual
 * target position (`target`) BEFORE the round-robin allocation even starts —
 * that result becomes SetupFly's "end" vector instead of p->end directly.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `&p->start`/`&p->end` are passed INLINE to GetVectorRotation/
 *    SearchItemTarget2 (no shared `pos` temp for these two calls) — `pos` only
 *    exists as the twins' usual tail-only temp (`pos = &p->start;` right
 *    before the coord.t[1]/[2] reads, same statement position as every
 *    twin). Caching `&p->start` in a named variable spanning BOTH the early
 *    calls and the tail forced it into a different callee-saved register
 *    than the pool cursor/`item`, swapping $s0/$s1 vs the twins' assignment —
 *    inlining the two early address-of expressions (still CSE'd by the
 *    compiler within that straight-line prefix) reproduced the twins' exact
 *    allocation.
 *  - GetVectorRotation's recovered `int *` signature gives `rx` and `ry`
 *    adjacent four-byte stack slots. GCC narrows their later assignments to
 *    the SVECTOR fields to low-halfword loads, matching the retail code.
 *  - The inlined allocator keeps a separate `slot` pseudo from the outer
 *    PSX.SYM `item`: `slot = items + COUNTER...;` is used throughout the
 *    loop/dispose block, and `item = slot;` is assigned exactly twice
 *    (the early-exit branch and right before the dispose block falls into
 *    `found:`) — the heavier tail (p/pos/param all needing registers across
 *    SetupFly) raises pressure the same way ReqItemLaunch's does.
 *  - `param = &item->param.arrow;` sits BEFORE the null check, same
 *    delay-slot lever as every twin; reused unchanged for both
 *    SetupFly's 1st arg and the final `param->count = 5;` store (unlike
 *    ReqItemLaunch's analogous store, which needed a FRESH direct
 *    `item->param.launch` access for its own scheduling tie — here the cached
 *    param reproduces the target directly).
 *  - `aowner`/`atype` temps for owner/type, same shape as the other twins (loaded
 *    back-to-back, stored owner/proc/mode/type in that order).
 *  - `item->collision.size = 0; item->model = ArrowModel;` immediately precede
 *    SetupFly, same position/interleaving as ReqItemLaunch.
 */
extern void ProcItemArrow(TItem *item);
extern Humanoid *SearchItemTarget2(Humanoid *owner, SVECTOR *rot,
                                   VECTOR *start, VECTOR *target);
extern void SetupFly(param_fly *param, VECTOR *start, VECTOR *end, s32 a4, s32 a5, s32 a6);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemArrow(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    TItem *slot;
    param_arrow *param;
    VECTOR *pos;
    Humanoid *aowner;
    s32 atype;
    SVECTOR dir;
    VECTOR target;
    int rx;
    int ry;
    s32 i;

    GetVectorRotation(&p->start, &p->end, &rx, &ry);
    dir.vz = 0;
    dir.vx = rx;
    dir.vy = ry;
    SearchItemTarget2(p->user, &dir, &p->start, &target);
    TAKE_ITEM_SLOT_VIA_CURSOR();
    param = &item->param.arrow;
    if (item == 0)
        return 0;
    aowner = p->user;
    atype = p->type;
    item->owner = aowner;
    item->proc = ProcItemArrow;
    item->mode = 0;
    item->type = atype;
    item->locate->locate.coord.t[0] = p->start.vx;
    pos = &p->start;
    item->locate->locate.coord.t[1] = pos->vy;
    item->locate->locate.coord.t[2] = pos->vz;
    item->locate->locate.super = 0;
    UpdateCoordinate(item->locate);
    item->collision.size = 0;
    item->model = ArrowModel;
    SetupFly(&param->fly, pos, &target, 0, FIXED_HALF, 300);
    param->count = 5;
    return 1;
}
