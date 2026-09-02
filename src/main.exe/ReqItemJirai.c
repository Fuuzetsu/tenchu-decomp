#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemJirai(struct PARAM_ITEM_DROP *p);
 *     ITEM.C:3609, 18 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_DROP * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_smoke * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       int atype
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
 * ReqItemJirai (0x80048958) — spawn a placed landmine/trap item ("jirai").
 * Twin of ReqItemDrop (same item TU, same pool round-robin on
 * ic and the same dispose-on-exhaustion block); unlike
 * ReqItemDrop there is no GetAreaMapLevel floor check — a jirai is placed
 * unconditionally. It gets ProcItemJirai as its processor and the trigger
 * velocity packed into param (the param.smoke member — ReqItemDrop uses
 * param.drop; identical param_korogari layout).
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `param = &item->param.smoke;` sits BEFORE the null check, same
 *    lever as ReqItemDrop (addiu fills the beqz delay slot).
 *  - `pos = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as ReqItemDrop (vy/vz reads go through pos; vx reads p directly).
 *  - aowner/atype are real temps, same as ReqItemDrop: the asm loads both
 *    p->user and p->type back-to-back before any owner/proc/mode/type stores.
 *  - x/y/z (end vector) ARE real temps: the asm batches three loads before
 *    three sh stores, matching ReqItemDrop's koro.vx/vy/vz shape exactly.
 *  - `item->param.smoke.koro.hint = 0;` uses the direct union path (not
 *    `param`) for this one store, same as ReqItemDrop.
 */
extern void ProcItemJirai(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemJirai(PARAM_ITEM_DROP *p)
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
        item->proc = ProcItemJirai;
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
    x = p->vec.vx;
    y = p->vec.vy;
    z = p->vec.vz;
    param->koro.vx = x;
    param->koro.vy = y;
    param->koro.vz = z;
    item->param.smoke.koro.hint = 0;
    param->koro.status = KORO_NORMAL;
    return 1;
}
