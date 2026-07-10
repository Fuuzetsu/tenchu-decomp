#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * STATUS: NON_MATCHING — item_use_gun was never CARVED (no `c` subsegment), so its
 * .c was never linked and never actually compared: its bytes came from the
 * raw data blob and matchdiff reported a false MATCH. Carved now, and the
 * draft below does not reproduce the original. matchdiff/progress refuse to
 * count an un-carved function from now on.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/item_use_gun", item_use_gun);
#else
/*
 * item_use_gun (0x80046854) — spawn a gun/firearm item. Same item TU, same
 * pool round-robin shape as ReqItemManebue/ReqItemKawarimi/etc, but:
 *  - returns void (no `return 1;`/`return 0;` — Ghidra's decompile shows a
 *    void-returning function; the epilogue never touches $v0)
 *  - instead of `it->model = D_8008E5BC[it->type];`, the tail block-copies
 *    the whole `p->end` VECTOR (4 words, align-4 word block move: 4x lw +
 *    4x sw — see the matching cookbook's "cast type's alignment drives copy
 *    code" rule) onto `it->param` (the union): `*(VECTOR *)it->param =
 *    p->end;`. Ghidra's rendering of this tail as bit-shifted short/char
 *    stores into a `param.gun` union disagrees with both m2c and the raw
 *    .s (plain lw/sw of all 4 words, no shifts) — trust the asm.
 */
extern void ProcItemGun(tag_TItem *item);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;

void item_use_gun(PARAM_ITEM_USE *p)
{
    tag_TItem *it;
    VECTOR *st;
    Humanoid *us;
    s32 ty;
    s32 i;

    i = 0;
    do
    {
        COUNTER_FOR_ITEM_ARRAY_++;
        if (0x1d < COUNTER_FOR_ITEM_ARRAY_)
            COUNTER_FOR_ITEM_ARRAY_ = 0;
        it = items + COUNTER_FOR_ITEM_ARRAY_;
        if (it->proc == 0)
            goto found;
        i++;
    } while (i < 0x1d);

    /* pool exhausted: force-dispose the slot the counter landed on */
    it->mode = 0xff;
    it->proc(it);
    DeleteConflict(it->locate);
    if (it->mode != 0)
    {
        AdtMessageBox(D_800121CC, it->type, (u32)it->mode);
    }
    it->owner = 0;
    it->proc = 0;

found:
    if (it == 0)
        return;
    us = p->user;
    ty = p->type;
    it->owner = us;
    it->proc = ProcItemGun;
    it->mode = 0;
    it->type = ty;
    it->locate->locate.coord.t[0] = p->start.vx;
    st = &p->start;
    it->locate->locate.coord.t[1] = st->vy;
    it->locate->locate.coord.t[2] = st->vz;
    it->locate->locate.super = 0;
    UpdateCoordinate(it->locate);
    it->coll_size = 0;
    *(VECTOR *)it->param = p->end;
}
#endif /* NON_MATCHING */
