#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * ReqItemMakibishi (0x8003f570) — spawn a thrown caltrop ("makibishi") item.
 * Twin of ReqItemDrop/ReqItemJirai/ReqItemDokudango/ReqItemKaengeki (same
 * item TU, same pool round-robin on COUNTER_FOR_ITEM_ARRAY_ and the same
 * dispose-on-exhaustion block); like ReqItemJirai/ReqItemDokudango there is
 * no GetAreaMapLevel floor check. It gets ProcItemMakibishi as its
 * processor, the throw velocity packed into param (param_korogari view,
 * same union member ReqItemDrop/ReqItemJirai/ReqItemDokudango use), then
 * plays a sound (SoundEx) at the drop origin before returning.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - The pool search uses a SEPARATE `cur` pointer from `it`: `cur = items +
 *    COUNTER_FOR_ITEM_ARRAY_;` in the loop/dispose block, then `it = cur;`
 *    assigned exactly twice — once inside the early-exit `if (cur->proc==0)`
 *    (paired with the `goto found;`), once right before the dispose block's
 *    final `it->owner=0; it->proc=0;`. Unlike the other twins (where the SAME
 *    `it` serves the whole function), this function's longer tail (`st`
 *    surviving to the final SoundEx call) raises register pressure enough
 *    that global-alloc gives `cur`/`it` DIFFERENT hard registers ($s0/$s1),
 *    making the assignment a real `move` instruction (confirmed: dropping
 *    the two-variable split and reusing one `it` throughout compiles 2
 *    instructions / 8 bytes SHORT — both `move` sites vanish as dead
 *    self-copies). The other twins almost certainly have this SAME two-
 *    pointer source shape; it's just invisible there because lower register
 *    pressure lets global-alloc color both pseudos into the SAME hard reg
 *    (self-copy, 0 bytes) — see the cookbook rule this taught.
 *  - `pp = (param_korogari *)it->param;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `st = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins; unlike them, `st` is READ AGAIN at the very end as
 *    SoundEx's location argument (`SoundEx(st, 0x22)`) — Ghidra mis-renders
 *    this call's first argument as `&(p->start).vy` (a bogus offset), but
 *    the raw asm shows a plain unmodified copy of the SAME register that
 *    holds `st` into $a0, several instructions before the call (the
 *    scheduler hoists the independent register copy early to fill the gap
 *    while it->model/end-vector loads execute) — it is just
 *    `SoundEx(st, 0x22);` written as the last statement before `return 1;`.
 *  - us/ty and x/y/z (end vector) are real temps, same shape as
 *    ReqItemJirai/ReqItemDokudango.
 *  - `((param_korogari *)it->param)->hint = 0;` re-casts it->param (not pp)
 *    for this one store, same as the other twins.
 */
extern void ProcItemMakibishi(tag_TItem *item);
extern void SoundEx(VECTOR *loc, int id);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;
/* Model pointer per item type. */
extern Sprite3D *D_8008E5BC[];

int ReqItemMakibishi(PARAM_ITEM_USE *p)
{
    tag_TItem *it;
    tag_TItem *cur;
    param_korogari *pp;
    VECTOR *st;
    Humanoid *us;
    s32 ty;
    s32 x;
    s32 y;
    s32 z;
    s32 i;

    i = 0;
    do
    {
        COUNTER_FOR_ITEM_ARRAY_++;
        if (0x1d < COUNTER_FOR_ITEM_ARRAY_)
            COUNTER_FOR_ITEM_ARRAY_ = 0;
        cur = items + COUNTER_FOR_ITEM_ARRAY_;
        if (cur->proc == 0)
        {
            it = cur;
            goto found;
        }
        i++;
    } while (i < 0x1d);

    /* pool exhausted: force-dispose the slot the counter landed on */
    cur->mode = 0xff;
    cur->proc(cur);
    DeleteConflict(cur->locate);
    if (cur->mode != 0)
    {
        AdtMessageBox(D_800121CC, cur->type, (u32)cur->mode);
    }
    it = cur;
    it->owner = 0;
    it->proc = 0;

found:
    pp = (param_korogari *)it->param;
    if (it == 0)
        return 0;
    us = p->user;
    ty = p->type;
    it->owner = us;
    it->proc = ProcItemMakibishi;
    it->mode = 0;
    it->type = ty;
    it->locate->locate.coord.t[0] = p->start.vx;
    st = &p->start;
    it->locate->locate.coord.t[1] = st->vy;
    it->locate->locate.coord.t[2] = st->vz;
    it->locate->locate.super = 0;
    UpdateCoordinate(it->locate);
    it->coll_size = 0;
    it->model = D_8008E5BC[it->type];
    x = p->end.vx;
    y = p->end.vy;
    z = p->end.vz;
    pp->vx = x;
    pp->vy = y;
    pp->vz = z;
    ((param_korogari *)it->param)->hint = 0;
    pp->status = 0;
    SoundEx(st, 0x22);
    return 1;
}
