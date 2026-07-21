#include "common.h"
#include "main.exe.h"

/*
 * ProcItemGosin (0x80041bf4) — the gosin (protection charm) item processor.
 * mode 0: play the use animation (0xF04) + sound 0x4C; mode 1: while the
 * animation plays, on completion (count==0 && loop) spray blood, set the
 * owner's active_item and start a 0x1C2-frame effect countdown — if the
 * animation was interrupted, toss the item back out (ReqItemDrop) and
 * dispose; mode 2: tick the countdown, spawning a FUN_8003944c flash every
 * 0x40 frames, dispose at 0.
 *
 * Matching notes (all verified against the original bytes; this is
 * ProcItemKusuri's shape — see that file for the buf[0x28]-with-casts and
 * drop-path conventions, ProcItemKawarimi/ProcItemGun for the dispose tail):
 *  - `ff` (u8 0xFF) is callee-saved ($s4): entry compare + the cross-jumped
 *    dispose tail's `item->mode = ff` (both copies of the duplicated tail
 *    must spell `ff` or they don't merge).
 *  - Real `switch` (fresh lbu + slti tree), bodies in source order 0,1,2;
 *    cases 0 and 1 end in a literal duplicated `item->mode = item->mode + 1;
 *    return;` cross-jumped into case 1's copy.
 *  - The dispatch index rides callee-saved $s0 because case 2 passes the
 *    literal `2` to FUN_8003944c after rand(): cse's record_jump_equiv on the
 *    `beq idx,2` taken edge substitutes the index register for the literal
 *    (the ProcItemGun rule).
 *  - Case 2 uses PSX.SYM's `param_gosin.count`. Retail reads that signed
 *    field through an explicit u16 memory view (`lhu`), then narrows through
 *    an s16 local (`sll/bnez` zero-test, not andi).
 *  - `*(VECTOR *)buf = D_80012248;` is a whole-VECTOR struct assignment (the
 *    16-byte batched-loads/stores block move), not four scalar assignments.
 *  - `human`/`itemID` (PSX.SYM's own names) are the drop path's load-batch
 *    temps; `owner->active_item = item->type` is the plain narrowing store
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

extern s16 SetNowMotion(Humanoid *human, s16 mid, s16 move);
extern void Sound(Humanoid *h, int id);
extern void NowReturnNormal(Humanoid *h);
extern void SetBleeds(VECTOR *pos, short grange, short srange, short n, int time, long col);
extern void FUN_8003944c(VECTOR *pos, ModelArchiveType *model, s32 a, s32 b, s32 col, s32 f, s32 rot, s32 h, s32 i, s32 j);
extern VECTOR D_80012248;

void ProcItemGosin(tag_TItem *item)
{
    u8 ff;
    u8 buf[0x28];

    ff = 0xff;
    if (item->mode == ff)
    {
        item->owner->active_item = 0;
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
            memset(buf, 0, sizeof(PARAM_ITEM_LAUNCH));
            ((PARAM_ITEM_LAUNCH *)buf)->type = itemID;
            ((PARAM_ITEM_LAUNCH *)buf)->user = human;
            ((PARAM_ITEM_LAUNCH *)buf)->start.vx = pos->vx;
            ((PARAM_ITEM_LAUNCH *)buf)->start.vy = pos->vy;
            ((PARAM_ITEM_LAUNCH *)buf)->start.vz = pos->vz;
            ((PARAM_ITEM_LAUNCH *)buf)->end.vx = rand() % 200 - 100;
            ((PARAM_ITEM_LAUNCH *)buf)->end.vy = rand() % 100 - 200;
            ((PARAM_ITEM_LAUNCH *)buf)->end.vz = rand() % 200 - 100;
            ReqItemDrop((PARAM_ITEM_LAUNCH *)buf);
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
        item->owner->active_item = item->type;
        *(u16 *)&((param_gosin *)item->param)->count = 0x1c2;
        item->mode = item->mode + 1;
        return;
    }

    case 2:
    {
        s16 c;

        c = *(u16 *)&((param_gosin *)item->param)->count - 1;
        *(u16 *)&((param_gosin *)item->param)->count = c;
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
        *(VECTOR *)buf = D_80012248;
        FUN_8003944c((VECTOR *)buf, item->owner->model, 0x1000, 0x6000, 0x808080, 0, (s16)(rand() % 0x168), 2, 0x78, 4);
        return;
    }
    }
}
