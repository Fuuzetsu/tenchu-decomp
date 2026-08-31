#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemGoshikimai(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2262, 21 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s2       struct param_goshikimai * param
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
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

/*
 * ReqItemGoshikimai (0x8004356c) — spawn a thrown "goshikimai" (five-colored
 * rice) item. Twin of ReqItemDrop/ReqItemJirai/ReqItemSmoke/ReqItemFire (same
 * item TU, same pool round-robin on ic and the same
 * dispose-on-exhaustion block); like ReqItemJirai/ReqItemSmoke there is no
 * GetAreaMapLevel floor check. It gets ProcItemGoshikimai as its processor.
 * Unlike the twins' rolling-item tail, this stores p->end into PSX.SYM's
 * `param_goshikimai.vec`, the three halfwords at offsets 0/2/4. There is no
 * hint/status/count tail.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `param = &item->param.goshikimai;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `pos = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins.
 *  - aowner/atype are real temps, same as the other twins.
 *  - The vec.vx store uses `item->param.goshikimai` directly (not `param`),
 *    so its base is $s0+0x20; vec.vy/vec.vz go through `param` ($s2).
 *  - All three stores are INLINE (no x/y/z temps): each is a single
 *    lhu-then-sh pair, interleaved in the asm (load, store, load, store,
 *    load, [store in the `return 1;` jump's delay slot]) — not the twins'
 *    batched-loads-then-stores shape.
 */
extern void ProcItemGoshikimai(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemGoshikimai(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_goshikimai *param;
    s32 i;

    TAKE_ITEM_SLOT();
    param = &item->param.goshikimai;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        aowner = p->user;
        atype = p->type;
        item->owner = aowner;
        item->proc = ProcItemGoshikimai;
        item->mode = 0;
        item->type = atype;
        item->locate->locate.coord.t[0] = p->start.vx;
        pos = &p->start;
        item->locate->locate.coord.t[1] = pos->vy;
        item->locate->locate.coord.t[2] = pos->vz;
        item->locate->locate.super = 0;
        UpdateCoordinate(item->locate);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    item->param.goshikimai.vec.vx = p->end.vx;
    param->vec.vy = p->end.vy;
    param->vec.vz = p->end.vz;
    return 1;
}
