#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemFire(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2706, 18 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_smoke * param
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
 * ReqItemFire (0x800456fc) — spawn a thrown fire item (near-clone of ReqItemSmoke: ProcItemFire, count=150). Twin
 * of ReqItemDrop/ReqItemJirai/ReqItemDokudango (same item TU, same pool
 * round-robin on ic and the same dispose-on-exhaustion
 * block); like ReqItemJirai/ReqItemDokudango there is no GetAreaMapLevel
 * floor check. It gets ProcItemFire as its processor, packs the throw
 * velocity into the embedded param_smoke.koro record, then initializes the
 * derived type's byte-sized count field to 150.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `param = &item->param.smoke;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `pos = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins.
 *  - aowner/atype and x/y/z are real temps, same shape as the other twins.
 *    The block-scoped `param_korogari *param` is PSX.SYM's second `param`.
 *  - `item->param.smoke.koro.hint = 0;` uses the direct union path (not
 *    `param`) for this one store, same as the other twins.
 */
extern void ProcItemFire(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemFire(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_smoke *param;
    s32 x;
    s32 y;
    s32 z;
    s32 i;

    TAKE_ITEM_SLOT();
    param = &item->param.smoke;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        aowner = p->user;
        atype = p->type;
        item->owner = aowner;
        item->proc = ProcItemFire;
        item->mode = ITEM_MODE_START;
        item->type = atype;
        item->locate->locate.coord.t[0] = p->start.vx;
        pos = &p->start;
        item->locate->locate.coord.t[1] = pos->vy;
        item->locate->locate.coord.t[2] = pos->vz;
        item->locate->locate.super = 0;
        UpdateCoordinate(item->locate);
        item->collision.size = 0;
        item->model.sprite = ItemImage[item->type];
    }
    {
        param_korogari *param;

        param = &item->param.smoke.koro;
        x = p->end.vx;
        y = p->end.vy;
        z = p->end.vz;
        setVector(param, x, y, z);
        item->param.smoke.koro.hint = 0;
        param->status = KORO_NORMAL;
    }
    param->count = 150;
    return 1;
}
