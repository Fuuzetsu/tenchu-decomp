#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * ReqItemGun (0x80046854) — spawn a gun/firearm item. Same item TU, same
 * pool round-robin shape as ReqItemManebue/ReqItemKawarimi/etc, but:
 *  - returns void (no `return 1;`/`return 0;` — Ghidra's decompile shows a
 *    void-returning function; the epilogue never touches $v0)
 *  - instead of `item->model = ItemImage[item->type];`, the tail block-copies
 *    the whole `p->end` VECTOR (4 words, align-4 word block move: 4x lw +
 *    4x sw — see the matching cookbook's "cast type's alignment drives copy
 *    code" rule) onto PSX.SYM's `param_gun.vec`. The raw assembly confirms
 *    plain lw/sw operations for all four words.
 */
extern void ProcItemGun(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

void ReqItemGun(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    VECTOR *pos;
    Humanoid *aowner;
    s32 atype;
    s32 i;

    TAKE_ITEM_SLOT();
    if (item == 0)
        return;
    aowner = p->user;
    atype = p->type;
    item->owner = aowner;
    item->proc = ProcItemGun;
    item->mode = ITEM_MODE_START;
    item->type = atype;
    item->locate->locate.coord.t[0] = p->start.vx;
    pos = &p->start;
    item->locate->locate.coord.t[1] = pos->vy;
    item->locate->locate.coord.t[2] = pos->vz;
    item->locate->locate.super = 0;
    UpdateCoordinate(item->locate);
    item->collision.size = 0;
    item->param.gun.vec = p->end;
}
