#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemMakibishi(struct PARAM_ITEM_DROP *p);
 *     ITEM.C:1235, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 *  - `param = (param_drop *)it->param;` sits BEFORE the null check, same
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
 *  - `((param_korogari *)it->param)->hint = 0;` re-casts it->param (not
 *    param) for this one store, same as the other twins.
 */
extern void ProcItemMakibishi(tag_TItem *item);
extern s16 SoundEx(VECTOR *loc, short id);
/* This TU defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/FRAMES (absolute here). */
extern s32 COUNTER_FOR_ITEM_ARRAY_;
/* Model pointer per item type. */
extern Sprite3D *ItemImage[];

int ReqItemMakibishi(PARAM_ITEM_LAUNCH *p)
{
    tag_TItem *it;
    tag_TItem *cur;
    param_drop *param;
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
    param = (param_drop *)it->param;
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
    it->model = (ModelType *)ItemImage[it->type];
    x = p->end.vx;
    y = p->end.vy;
    z = p->end.vz;
    param->koro.vx = x;
    param->koro.vy = y;
    param->koro.vz = z;
    ((param_korogari *)it->param)->hint = 0;
    param->koro.status = 0;
    SoundEx(st, 0x22);
    return 1;
}
