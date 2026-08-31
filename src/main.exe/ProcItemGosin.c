#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "sound.h"

/*
 * ProcItemGosin (0x80041bf4) — the gosin (protection charm) item processor.
 * mode 0: play the use animation (0xF04) + sound 0x4C; mode 1: while the
 * animation plays, on completion (count==0 && loop) spray blood, set the
 * owner's itmctl and start a 0x1C2-frame effect countdown — if the
 * animation was interrupted, toss the item back out (ReqItemDrop) and
 * dispose; mode 2: tick the countdown, spawning a set_impact_ex_ flash every
 * 0x40 frames, dispose at 0.
 *
 * Matching notes (all verified against the original bytes; this is
 * ProcItemKusuri's shape — see that file for the shared stack scratch and
 * drop-path conventions, ProcItemKawarimi/ProcItemGun for the dispose tail):
 *  - `ITEM_MODE_DISPOSE` (u8 ITEM_MODE_DISPOSE) is callee-saved ($s4): entry compare + the
 *    cross-jumped dispose tail's `item->mode = ITEM_MODE_DISPOSE` (both copies of the
 *    duplicated tail must spell `ITEM_MODE_DISPOSE` or they don't merge).
 *  - Real `switch` (fresh lbu + slti tree), bodies in source order 0,1,2;
 *    cases 0 and 1 end in a literal duplicated `item->mode = item->mode + 1;
 *    return;` cross-jumped into case 1's copy.
 *  - The dispatch index rides callee-saved $s0 because case 2 passes the
 *    literal `2` to set_impact_ex_ after rand(): cse's record_jump_equiv on the
 *    `beq idx,2` taken edge substitutes the index register for the literal
 *    (the ProcItemGun rule).
 *  - Case 2 uses PSX.SYM's `param_gosin.count`. Retail changed the demo's
 *    signed field to `u16` (`lhu`), then narrows through an s16 local
 *    (`sll/bnez` zero-test, not andi).
 *  - `scratch.v = vec_y_n1200_z_400;` is a whole-VECTOR struct assignment (the
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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
extern void set_impact_ex_(VECTOR *pos, GsCOORDINATE2 *super,
                           short start_size, short end_size,
                           long start_color, long end_color,
                           /* s32 tail vs the definition's u16s is measured
                            * byte-required (same negative-constant lever as
                            * ProcItemShinsoku.c). */
                           s32 rotate, s32 rotate_speed, s32 time, s32 type);
extern VECTOR vec_y_n1200_z_400; /* {0,-1200,400} */

void ProcItemGosin(TItem *item)
{
    enum
    {
        GOSIN_MODE_START = 0,
        GOSIN_MODE_WAIT = 1,
        GOSIN_MODE_ACTIVE = 2
    };
    ProcItemGosinScratch scratch;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->owner->itmctl = 0;
        item->mode = GOSIN_MODE_START;
        return;
    }
    switch (item->mode)
    {
    case GOSIN_MODE_START:
        SetNowMotion(item->owner, MOT_ITEM_KAENGEKI, 1);
        Sound(item->owner, SE_ITEM_USE);
        item->mode++;
        return;

    case GOSIN_MODE_WAIT:
    {
        MotionManager *mot;

        mot = item->owner->motion;
        if (mot->mid != MOT_ITEM_KAENGEKI)
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
            item->mode = ITEM_MODE_DISPOSE;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != GOSIN_MODE_START)
            {
                AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
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
        SetBleeds(GetAbsolutePosition(item->owner->model->object[1], 0, 0, 0), 600, 100, 20, 15, RGB24(180, 140, 30));
        item->owner->itmctl = item->type;
        item->param.gosin.count = GOSIN_DURATION;
        item->mode++;
        return;
    }

    case GOSIN_MODE_ACTIVE:
    {
        s16 c;

        c = item->param.gosin.count - 1;
        item->param.gosin.count = c;
        if (c == 0)
        {
            if (item->proc == 0)
                return;
            item->mode = ITEM_MODE_DISPOSE;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != GOSIN_MODE_START)
            {
                AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
            return;
        }
        if ((c & 0x3f) != 0)
            return;
        scratch.v = vec_y_n1200_z_400;
        set_impact_ex_(&scratch.v, &item->owner->model->locate,
                       FIXED_ONE, 6 * FIXED_ONE, COLOR_GRAY, 0,
                       (s16)(rand() % 360), 2, 120, 4);
        return;
    }
    }
}
