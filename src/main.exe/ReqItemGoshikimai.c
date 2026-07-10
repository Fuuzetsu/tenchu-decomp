#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemGoshikimai(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2262, 21 src lines, frame 32 bytes, saved-reg mask 0x80070000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
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
 * END PSX.SYM */

/*
 * ReqItemGoshikimai (0x8004356c) — spawn a thrown "goshikimai" (five-colored
 * rice) item. Twin of ReqItemDrop/ReqItemJirai/ReqItemSmoke/ReqItemFire (same
 * item TU, same pool round-robin on COUNTER_FOR_ITEM_ARRAY_ and the same
 * dispose-on-exhaustion block); like ReqItemJirai/ReqItemSmoke there is no
 * GetAreaMapLevel floor check. It gets ProcItemGoshikimai as its processor.
 * Unlike the twins' vx/vy/vz-into-param_korogari tail, tools/access.py shows
 * this one stores only THREE plain 16-bit values at param OFFSETS 0/2/4 (not
 * param_korogari's vx@4/vy@6/vz@8) — a distinct, narrower union member
 * reached via explicit offset casts off the same proven `it->param`/`pp`
 * pointer — and there is no hint=0/status=0/count=N tail at all.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `pp = (param_korogari *)it->param;` sits BEFORE the null check, same
 *    lever as the other twins (addiu fills the beqz delay slot) — even
 *    though pp's own named fields are never read in this function.
 *  - `st = &p->start;` materialized between the t[0] and t[1] stores, same
 *    as the other twins.
 *  - us/ty are real temps, same as the other twins.
 *  - The offset-0 store re-casts `it->param` fresh (not `pp`) — same idiom
 *    as the twins' `((param_korogari *)it->param)->hint = 0;` (asm: base
 *    reg $s0 + 0x20, not $s2) — while offsets 2 and 4 go through `pp` ($s2).
 *  - All three stores are INLINE (no x/y/z temps): each is a single
 *    lhu-then-sh pair, interleaved in the asm (load, store, load, store,
 *    load, [store in the `return 1;` jump's delay slot]) — not the twins'
 *    batched-loads-then-stores shape.
 */
extern void ProcItemGoshikimai(tag_TItem *item);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;
/* Model pointer per item type. */
extern Sprite3D *D_8008E5BC[];

int ReqItemGoshikimai(PARAM_ITEM_USE *p)
{
    tag_TItem *it;
    param_korogari *pp;
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
    pp = (param_korogari *)it->param;
    if (it == 0)
        return 0;
    us = p->user;
    ty = p->type;
    it->owner = us;
    it->proc = ProcItemGoshikimai;
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
    *(u16 *)it->param = p->end.vx;
    *(u16 *)((u8 *)pp + 2) = p->end.vy;
    *(u16 *)((u8 *)pp + 4) = p->end.vz;
    return 1;
}
