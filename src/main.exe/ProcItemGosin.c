#include "common.h"
#include "main.exe.h"

/*
 * ProcItemGosin (0x80041bf4) — the gosin (protection charm) item processor.
 * mode 0: play the use animation (0xF04) + sound 0x4C; mode 1: while the
 * animation plays, on completion (count==0 && loop) spray blood, set the
 * owner's itmctl and start a 0x1C2-frame effect countdown — if the
 * animation was interrupted, toss the item back out (ReqItemDrop) and
 * dispose; mode 2: tick the countdown, spawning a FUN_8003944c flash every
 * 0x40 frames, dispose at 0.
 *
 * Matching notes (all verified against the original bytes; this is
 * ProcItemKusuri's shape — see that file for the shared stack scratch and
 * drop-path conventions, ProcItemKawarimi/ProcItemGun for the dispose tail):
 *  - `ff` (u8 ITEM_MODE_DISPOSE) is callee-saved ($s4): entry compare + the
 *    cross-jumped dispose tail's `item->mode = ff` (both copies of the
 *    duplicated tail must spell `ff` or they don't merge).
 *  - Real `switch` (fresh lbu + slti tree), bodies in source order 0,1,2;
 *    cases 0 and 1 end in a literal duplicated `item->mode = item->mode + 1;
 *    return;` cross-jumped into case 1's copy.
 *  - The dispatch index rides callee-saved $s0 because case 2 passes the
 *    literal `2` to FUN_8003944c after rand(): cse's record_jump_equiv on the
 *    `beq idx,2` taken edge substitutes the index register for the literal
 *    (the ProcItemGun rule).
 *  - Case 2 uses PSX.SYM's `param_gosin.count`. Retail changed the demo's
 *    signed field to `u16` (`lhu`), then narrows through an s16 local
 *    (`sll/bnez` zero-test, not andi).
 *  - `scratch.v = D_80012248;` is a whole-VECTOR struct assignment (the
 *    16-byte batched-loads/stores block move), not four scalar assignments.
 *  - `human`/`itemID` (PSX.SYM's own names) are the drop path's load-batch
 *    temps; `owner->itmctl = item->type` is the plain narrowing store
 *    (lhu of the s32 type field).
 */
/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemGosin(struct tag_TItem *item);
 *     ITEM.C:1804, 52 src lines, frame 88 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s3       struct tag_TItem * item
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+24     struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+24     struct VECTOR v
 * END PSX.SYM */

#include "item.h"

typedef union
{
    PARAM_ITEM_LAUNCH p;
    VECTOR v;
} ProcItemGosinScratch;

/* Retail's caller promotes these scalar arguments before the call. */
extern void FUN_8003944c(VECTOR *pos, GsCOORDINATE2 *super,
                         s32 start_size, s32 end_size,
                         s32 start_color, s32 end_color,
                         s32 rotate, s32 rotate_speed, s32 time, s32 type);
extern VECTOR D_80012248;

void ProcItemGosin(TItem *item)
{
    u8 ff;
    ProcItemGosinScratch scratch;

    ff = ITEM_MODE_DISPOSE;
    if (item->mode == ff)
    {
        item->owner->itmctl = 0;
        item->mode = 0;
        return;
    }
    switch (item->mode)
    {
    case 0:
        SetNowMotion(item->owner, 0xf04, 1);
        Sound(item->owner, 0x4c);
        item->mode = item->mode + 1;
        return;

    case 1:
    {
        MotionManager *mot;

        mot = item->owner->motion;
        if (mot->mid != 0xf04)
        {
            VECTOR *pos;
            Humanoid *human;
            s32 itemID;

            pos = GetAbsolutePosition(item->locate, 0, 0, 0);
            human = item->owner;
            itemID = item->type;
            memset(&scratch.p, 0, sizeof(PARAM_ITEM_LAUNCH));
            scratch.p.type = itemID;
            scratch.p.user = human;
            scratch.p.start.vx = pos->vx;
            scratch.p.start.vy = pos->vy;
            scratch.p.start.vz = pos->vz;
            scratch.p.end.vx = rand() % 200 - 100;
            scratch.p.end.vy = rand() % 100 - 200;
            scratch.p.end.vz = rand() % 200 - 100;
            ReqItemDrop(&scratch.p);
            if (item->proc == 0)
                return;
            item->mode = ff;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
            return;
        }
        if (mot->count != 0)
            return;
        if (mot->loop == 0)
            return;
        NowReturnNormal(item->owner);
        SetBleeds(GetAbsolutePosition(item->owner->model->object[1], 0, 0, 0), 600, 100, 20, 15, 0xB48C1E);
        item->owner->itmctl = item->type;
        item->param.gosin.count = 0x1c2;
        item->mode = item->mode + 1;
        return;
    }

    case 2:
    {
        s16 c;

        c = item->param.gosin.count - 1;
        item->param.gosin.count = c;
        if (c == 0)
        {
            if (item->proc == 0)
                return;
            item->mode = ff;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
            return;
        }
        if ((c & 0x3f) != 0)
            return;
        scratch.v = D_80012248;
        FUN_8003944c(&scratch.v, &item->owner->model->locate,
                     0x1000, 0x6000, 0x808080, 0,
                     (s16)(rand() % 0x168), 2, 0x78, 4);
        return;
    }
    }
}
