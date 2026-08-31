#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemNinken(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2461, 21 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_ninken * param
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
 *     reg   $a0       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

/*
 * ReqItemNinken (0x800446d0) — spawn a ninken (tracker dog) item. Twin of
 * ReqItemDrop/ReqItemJirai/ReqItemDokudango/ReqItemSmoke/ReqItemFire (same
 * item TU, same pool round-robin on ic and the same
 * dispose-on-exhaustion block); like its siblings there is no
 * GetAreaMapLevel floor check. It gets ProcItemNinken as its processor, packs
 * the throw velocity into the embedded param_ninken.koro record but — unlike
 * them — only reads end.vx/end.vz from the caller: end.vy is never loaded,
 * and koro.vy instead gets the hardcoded
 * constant -250 (a fixed vertical/launch parameter for the tracker dog).
 * PSX.SYM identifies the following word and halfword as the slave and count
 * fields of param_ninken, respectively.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `param = &item->param.ninken;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `pos = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins.
 *  - aowner/atype and x/z are real temps, same shape as the other twins (no `y`
 *    temp here: end.vy is never read). The block-scoped
 *    `koro` is PSX.SYM's second `param`.
 *  - `item->param.ninken.koro.hint = 0;` uses the direct union path (not
 *    `param`) for this one store, same as the other twins.
 */
extern void ProcItemNinken(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemNinken(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_ninken *param;
    VECTOR *pos;
    Humanoid *aowner;
    s32 atype;
    s32 x;
    s32 z;
    s32 i;

    TAKE_ITEM_SLOT();
    param = &item->param.ninken;
    if (item == 0)
        return 0;
    aowner = p->user;
    atype = p->type;
    item->owner = aowner;
    item->proc = ProcItemNinken;
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
    {
        param_korogari *koro; /* second pointer pseudo, byte-required (writing through the full member path re-colors the stores) */

        koro = &item->param.ninken.koro;
        x = p->end.vx;
        z = p->end.vz;
        koro->vx = x;
        koro->vy = -250;
        koro->vz = z;
        item->param.ninken.koro.hint = 0;
        koro->status = KORO_NORMAL;
    }
    param->slave = 0;
    param->count = 15;
    SetNowMotion(item->owner, MOT_ITEM_THROW, 1);
    return 1;
}
