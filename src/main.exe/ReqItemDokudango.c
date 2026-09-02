#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemDokudango(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:1777, 23 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_dokudango * param
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
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

/*
 * ReqItemDokudango (0x80041a34) — spawn a thrown poison-dumpling
 * ("dokudango") item. Twin of ReqItemDrop/ReqItemJirai (same item TU, same
 * pool round-robin on ic and the same
 * dispose-on-exhaustion block); like ReqItemJirai there is no
 * GetAreaMapLevel floor check. It gets ProcItemDokudango as its processor,
 * packs the throw velocity into the embedded param_dokudango.koro record,
 * clears eater, initializes count, then calls SetNowMotion on the user.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `param = &item->param.dokudango;` sits BEFORE the null check, same
 *    lever as ReqItemDrop/ReqItemJirai (addiu fills the beqz delay slot).
 *  - `pos = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as ReqItemDrop/ReqItemJirai.
 *  - aowner/atype and x/y/z are real temps, same shape as
 *    ReqItemDrop/ReqItemJirai. The block-scoped `param_korogari *param`
 *    shadow is the second `param` recorded by PSX.SYM.
 *  - `item->param.dokudango.koro.hint = 0;` uses the direct union path (not
 *    `param`) for this one store, same as ReqItemDrop/ReqItemJirai.
 *  - PSX.SYM supplies the param/eater/org_think/count nesting and names. The
 *    retail executable accesses count as a halfword, widened from the demo's
 *    byte, so the retail declaration retains that one version difference.
 */
extern void ProcItemDokudango(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemDokudango(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_dokudango *param;
    s32 x;
    s32 y;
    s32 z;
    s32 i;

    TAKE_ITEM_SLOT();
    param = &item->param.dokudango;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemDokudango);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    {
        param_korogari *param; /* shadows the outer `param`, as PSX.SYM has it;
                                * byte-required -- writing through the full
                                * member path re-colors the stores */

        param = &item->param.dokudango.koro;
        x = p->end.vx;
        y = p->end.vy;
        z = p->end.vz;
        setVector(param, x, y, z);
        item->param.dokudango.koro.hint = 0;
        param->status = KORO_NORMAL;
    }
    param->count = 10;
    param->eater = 0;
    SetNowMotion(item->owner, MOT_ITEM_THROW, MOTION_MOVE_APPLY);
    return 1;
}
