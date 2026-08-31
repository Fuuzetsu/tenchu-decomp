#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemKusuri(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:1549, 18 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_drop * param
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
 * ReqItemKusuri (0x80040a98) — spawn a "kusuri" (medicine/health potion)
 * item. Same item-TU pool round-robin on ic and the same
 * dispose-on-exhaustion block as ReqItemJirai/ReqItemShinsoku; unlike those
 * twins, kusuri is a plain "use" item with no rolling/placement physics: it
 * never touches item->param at all (no param_korogari view, no end-vector
 * store, no hint/status/count) — confirmed by tools/access.py, whose last
 * body access is the ItemImage[item->type] model load, immediately followed
 * by the epilogue register reloads. It gets ProcItemKusuri as its processor
 * and returns 1 on the normal path (like Jirai; Shinsoku is the outlier that
 * returns 0 on both paths).
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `pos = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as Jirai/Shinsoku.
 *  - aowner/atype are real temps, same shape as the other twins.
 *  - `item->locate` is NOT cached into a pointer temp: it is reloaded fresh at
 *    every one of its 5 textual uses (t[0]/t[1]/t[2]/super/UpdateCoordinate
 *    argument) — access.py shows 5 separate `lw $s0,0x10` reloads. The very
 *    first of those reloads is scheduled by cc1 BEFORE the proc/mode/type
 *    stores even though it textually follows them in source (independent
 *    loads get hoisted ahead of independent stores); write the natural
 *    owner/proc/mode/type/coord order and let the scheduler do this, exactly
 *    like Jirai.
 *  - no `pp`/param_korogari view at all — this function has nothing to do
 *    with item->param.
 */
extern void ProcItemKusuri(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemKusuri(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    VECTOR *pos;
    Humanoid *aowner;
    s32 atype;
    s32 i;

    TAKE_ITEM_SLOT();
    if (item == 0)
        return 0;
    aowner = p->user;
    atype = p->type;
    item->owner = aowner;
    item->proc = ProcItemKusuri;
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
    return 1;
}
