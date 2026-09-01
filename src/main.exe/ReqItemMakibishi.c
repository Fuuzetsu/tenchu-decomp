#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemMakibishi(struct PARAM_ITEM_DROP *p);
 *     ITEM.C:1235, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_DROP * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_drop * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       int atype
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
 * ReqItemMakibishi (0x8003f570) — spawn a thrown caltrop ("makibishi") item.
 * Twin of ReqItemDrop/ReqItemJirai/ReqItemDokudango/ReqItemKaengeki (same
 * item TU, same pool round-robin on ic and the same
 * dispose-on-exhaustion block); like ReqItemJirai/ReqItemDokudango there is
 * no GetAreaMapLevel floor check. It gets ProcItemMakibishi as its
 * processor, the throw velocity packed into param (param_korogari view,
 * same union member ReqItemDrop/ReqItemJirai/ReqItemDokudango use), then
 * plays a sound (SoundEx) at the drop origin before returning.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - The inlined allocator uses PSX.SYM's SEPARATE `ret` pointer from the
 *    outer `item`: `ret = items + ic;` in the loop/dispose block, then
 *    `item = ret;` assigned exactly twice — once inside the early-exit
 *    `if (ret->proc==0)`
 *    (paired with the `goto found;`), once right before the dispose block's
 *    final `item->owner=0; item->proc=0;`. Unlike the other twins (where the
 *    SAME `item` serves the whole function), this function's longer tail (`pos`
 *    surviving to the final SoundEx call) raises register pressure enough
 *    that global-alloc gives `ret`/`item` DIFFERENT hard registers ($s0/$s1),
 *    making the assignment a real `move` instruction (confirmed: dropping
 *    the two-variable split and reusing one `item` throughout compiles 2
 *    instructions / 8 bytes SHORT — both `move` sites vanish as dead
 *    self-copies). The other twins almost certainly have this SAME two-
 *    pointer source shape; it's just invisible there because lower register
 *    pressure lets global-alloc color both pseudos into the SAME hard reg
 *    (self-copy, 0 bytes) — see the cookbook rule this taught.
 *  - `param = &item->param.drop;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot).
 *  - `pos = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins; unlike them, `pos` is READ AGAIN at the very end as
 *    SoundEx's location argument (`SoundEx(pos, 0x22)`) — Ghidra mis-renders
 *    this call's first argument as `&(p->start).vy` (a bogus offset), but
 *    the raw asm shows a plain unmodified copy of the SAME register that
 *    holds `pos` into $a0, several instructions before the call (the
 *    scheduler hoists the independent register copy early to fill the gap
 *    while item->model/end-vector loads execute) — it is just
 *    `SoundEx(pos, 0x22);` written as the last statement before `return 1;`.
 *  - aowner/atype and x/y/z (end vector) are real temps, same shape as
 *    ReqItemJirai/ReqItemDokudango.
 *  - `item->param.drop.koro.hint = 0;` uses the direct union path (not
 *    `param`) for this one store, same as the other twins.
 */
extern void ProcItemMakibishi(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemMakibishi(PARAM_ITEM_DROP *p)
{
    TItem *item;
    TItem *ret;
    param_drop *param;
    VECTOR *pos;
    s32 x;
    s32 y;
    s32 z;
    s32 i;

    TAKE_ITEM_SLOT_VIA_CURSOR();
    param = &item->param.drop;
    if (item == 0)
        return 0;
    {
        Humanoid *aowner;
        s32 atype;

        aowner = p->user;
        atype = p->type;
        item->owner = aowner;
        item->proc = ProcItemMakibishi;
        item->mode = ITEM_MODE_START;
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
    x = p->vec.vx;
    y = p->vec.vy;
    z = p->vec.vz;
    param->koro.vx = x;
    param->koro.vy = y;
    param->koro.vz = z;
    item->param.drop.koro.hint = 0;
    param->koro.status = KORO_NORMAL;
    SoundEx(pos, SE_CALTROP_THROW);
    return 1;
}
