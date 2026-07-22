#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "afterimage.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemHappou(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2541, 41 src lines, frame 72 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
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
 *     param $s4       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s5       int i
 *     reg   $s1       struct tag_TItem * item
 *     reg   $s3       struct param_launch * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $s2       struct VECTOR * pos
 *     stack sp+24     struct SVECTOR rot
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * ReqItemHappou (0x80044b18) — fire an 8-shot "happou" volley (8 homing
 * projectiles fanned out by a random jitter on the aim direction). Twin of
 * ReqItemArrow/ReqItemLaunch (same item TU, same pool round-robin on
 * ic, same dispose-on-exhaustion block, same
 * slot/item two-pseudo pool search, same SetupFly+SetupAfterimage tail as
 * ReqItemLaunch); unlike every other twin the OUTER shape is a
 * while(1)+break loop firing the whole allocate-and-launch sequence 8 times
 * (SetNowMotion once up front, Sound once after all 8, `p->end` reused as
 * scratch OUTPUT storage for SearchItemTarget2 on every iteration). Unlike
 * ReqItemArrow/Launch, `item->model` is never touched at all (this item has no
 * visible sprite, only the afterimage trail), and SetupAfterimage's first
 * arg is `item->locate` — not the optional visual `item->model` used by
 * ReqItemLaunch's call to the same function.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - The outer loop is the cookbook's `while (1) { if (!(cond)) break; ...}`
 *    shape: `i < 8` is a real TOP test (not folded away like a provable-
 *    constant `for`/`while` would be) because loop.c only rotates a
 *    `NOTE_INSN_LOOP_BEG` immediately followed by a simplejump — the
 *    condjump-first shape here keeps the top test AND still gets invariant
 *    hoisting: `items + ic`'s `&items[0]` half is
 *    hoisted all the way past BOTH loop levels (computed once, before the
 *    outer loop, from the same unmodified `slot = items + COUNTER...;`
 *    expression the single-shot twins use — no separate cached-base
 *    variable needed in source).
 *  - The inlined allocator keeps `slot` separate from PSX.SYM's outer
 *    `item`, with the stub-jump early-exit pattern confirmed in the raw .s.
 *    Its block-scoped `i` shadows the exact outer volley counter `i`, just
 *    as the two PSX.SYM local records do.
 *  - `param = &item->param.launch;` before the null check, same lever
 *    as every twin; reused for SetupFly and the two final member stores.
 *  - `pos = &p->start;` sits in the SAME position as every twin (right after
 *    the direct `p->start.vx` read, before the pos->vy/pos->vz reads) and,
 *    unlike ReqItemArrow, stays live (no register reuse forcing a
 *    recompute) all the way to SetupFly's 2nd arg — reused 4 times total.
 *  - `item->locate->rotate = p->user->model->rotate;` and the local `rot =
 *    p->user->model->rotate;` are TWO INDEPENDENT struct copies, each
 *    re-reading `p->user->model` fresh (confirmed: two separate `lw` pairs
 *    in the raw .s, not a cached pointer) — an SVECTOR (align-2) copy
 *    compiles to `lwl/lwr`+`swl/swr` word pairs per the cookbook's
 *    alignment-drives-copy-code rule.
 *  - The random jitter is a plain signed `r % (R * 2)`: `rot.vy = rot.vy +
 *    (r % (R * 2) - R);` reassociates via fold-const.c into the target's
 *    `rot.vy - 0x100 + r%512` layout (same "A - C + B" lever as the
 *    cookbook's `rand() % 1000 - 500` example) and the sign-correcting
 *    bias/shift dance for a negative dividend is entirely automatic from
 *    the natural `%` (no hand-rolled staging needed).
 *  - `en = &p->end;` computed once, reused for both SearchItemTarget2's 4th
 *    arg and SetupFly's 3rd arg (same "computed once, read multiple times"
 *    shape as `pos`).
 *  - **`i++;` (the outer counter) belongs AFTER `SetupFly`, not textually
 *    before `SearchItemTarget2` where Ghidra renders it** (found via the
 *    permuter after manual placement stalled at CURRENT(46) — a pure
 *    register-allocation tie, not a structural difference). Ghidra/the raw
 *    `.s` show the increment in `SearchItemTarget2`'s call delay slot, which
 *    reads as "the statement right before this call" — but that's just
 *    where reorg happened to schedule it FROM; the true source statement is
 *    several lines later, and reorg pulled the independent increment
 *    BACKWARD across the SearchItemTarget2 call into that delay slot. Don't
 *    trust a delay-slot instruction's apparent position as its source
 *    position — only that it's independent of the call's own effects.
 *  - No `item->model` store at all (unlike ReqItemArrow/Launch) — this item
 *    has no sprite.
 *  - `SetupAfterimage(item->locate, 10)` uses the item's coordinate model,
 *    whereas ReqItemLaunch passes its visual model. Both are `ModelType *`,
 *    matching SetupAfterimage's PSX.SYM signature.
 *  - The afterimage vector constants (0x1e/-0x1e, vy=vz=0) and struct shape
 *    are identical to ReqItemLaunch's; only the trailing count (8, not 5)
 *    differs.
 */
extern void ProcItemHappou(TItem *item);
extern Humanoid *SearchItemTarget2(Humanoid *owner, SVECTOR *rot,
                                   VECTOR *start, VECTOR *target);
extern void SetupFly(param_fly *param, VECTOR *start, VECTOR *end, s32 a4, s32 a5, s32 a6);
extern int rand(void);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemHappou(PARAM_ITEM_LAUNCH *p)
{
    enum
    {
        R = 256
    };
    TItem *item;
    TItem *slot;
    param_launch *param;
    VECTOR *pos;
    VECTOR *en;
    Humanoid *aowner;
    s32 atype;
    AfterimageType *ai;
    SVECTOR rot;
    s32 r;
    s32 i;

    SetNowMotion(p->user, 0xf02, 1);
    i = 0;
    while (1)
    {
        if (!(i < 8))
            break;
        {
            s32 i;

            i = 0;
            do
            {
                ic++;
                if (0x1d < ic)
                    ic = 0;
                slot = items + ic;
                if (slot->proc == 0)
                {
                    item = slot;
                    goto found;
                }
                i++;
            } while (i < 0x1d);

            /* pool exhausted: force-dispose the slot the counter landed on */
            slot->mode = ITEM_MODE_DISPOSE;
            slot->proc(slot);
            DeleteConflict(slot->locate);
            if (slot->mode != 0)
            {
                AdtMessageBox(D_800121CC, slot->type, (u32)slot->mode);
            }
            item = slot;
            item->owner = 0;
            item->proc = 0;
        }

    found:
        param = &item->param.launch;
        if (item == 0)
            return 0;
        aowner = p->user;
        atype = p->type;
        item->owner = aowner;
        item->proc = ProcItemHappou;
        item->mode = 0;
        item->type = atype;
        item->locate->locate.coord.t[0] = p->start.vx;
        pos = &p->start;
        item->locate->locate.coord.t[1] = pos->vy;
        item->locate->locate.coord.t[2] = pos->vz;
        item->locate->locate.super = 0;
        UpdateCoordinate(item->locate);
        item->collision.size = 0;
        item->locate->rotate = p->user->model->rotate;
        rot = p->user->model->rotate;
        r = rand();
        rot.vy = rot.vy + (r % (R * 2) - R);
        en = &p->end;
        SearchItemTarget2(p->user, &rot, pos, en);
        SetupFly(&param->fly, pos, en, 0x1000, 0x400, 0x190);
        i++;
        ai = SetupAfterimage(item->locate, 10);
        param->effect = ai;
        ai->vector1.vx = 0x1e;
        ai->vector1.vy = 0;
        ai->vector1.vz = 0;
        ai->vector2.vx = -0x1e;
        ai->vector2.vy = 0;
        ai->vector2.vz = 0;
        param->count = 8;
    }
    Sound(p->user, 0x4c);
    return 1;
}
