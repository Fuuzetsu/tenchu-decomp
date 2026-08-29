#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemHenshin(struct tag_TItem *item);
 *     ITEM.C:2060, 140 src lines, frame 208 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     reg   $s4       struct param_henshin * param
 *     reg   $a1       int i
 *     reg   $s1       struct ModelArchiveType * mad
 *     reg   $a2       struct ModelArchiveType * hen
 *     stack sp+16     struct ModelArchiveType *[30] target
 *     reg   $s2       int targets
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+136    struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+136    struct SVECTOR sv
 *     stack sp+144    struct SVECTOR sv
 *     reg   $s3       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern long EmergencyNotice;
 * END PSX.SYM */

/*
 * The Henshin (disguise) item processor: snapshots the player's model into
 * HenshinSnapshot, swaps in the disguise character for HenshinCount (600)
 * frames with a smoke puff at both ends, ticks the countdown each frame,
 * and restores the original model when the timer runs out, damage breaks
 * the disguise, or the item is disposed (HenshinItem marks the active
 * instance).
 */
#include "item.h"

typedef union
{
    PARAM_ITEM_LAUNCH p;
    SVECTOR sv;
} ProcItemHenshinScratch;

/*
 * MATCH.
 *
 * ProcItemHenshin (0x80042c1c) owns the disguise transformation.  It starts
 * and monitors motion 0xf04, drops itself when that motion is interrupted,
 * swaps the owner's model-object data to/from two saved snapshots, and tears
 * down the previous disguise before installing a new one.
 *
 * Matching notes:
 *  - The retail snapshot begins with the saved `waist` value, followed by
 *    ordinary 12-byte model-part records. Indexing the nested `p` array by
 *    the loop's own counter gives loop.c one unbiased
 *    induction pointer and the target's natural +4/+8/+10/+12 offsets.
 *  - HenshinItem and HenshinCount use volatile views only to preserve the
 *    original observable load/store sequence.  In particular, the restore
 *    path stores the current-disguise pointer before reloading item->owner,
 *    and mode 2 finishes its mode/count stores before loading owner/type.
 *    The old disguise pointer is copied once before its null/proc checks so
 *    volatility does not introduce redundant global reloads.
 *  - `scratch` is the exact sp+0x10..0x37 lifetime overlay: PSX.SYM records
 *    PARAM_ITEM_LAUNCH `p` on the interrupted-motion path and SVECTOR `sv` on
 *    the smoke paths.
 *  - Case 0 deliberately does not assign HenshinItem.  It jumps directly to
 *    the shared mode increment; only the completed mode-1 path installs the
 *    current item after disposing any prior disguise.
 */
extern TItem *volatile HenshinItem;
extern volatile u16 HenshinCount;
extern SVECTOR svec_y_n50[]; /* {0,-50,0} */

void ProcItemHenshin(TItem *item)
{
    Humanoid *human;
    ModelArchiveType *mad;
    u8 ff;
    ProcItemHenshinScratch scratch;

    human = item->owner;
    mad = human->model;
    ff = ITEM_MODE_DISPOSE;

    if (item->mode == ff)
    {
        if (item == HenshinItem)
        {
            s32 i;
            HenshinModelSnapshot *saved;

            i = 0;
            saved = &Item_save;
            mad->rotate.pad = (s16)saved->waist;
            if (mad->n > 0)
            {
                do
                {
                    mad->object[i]->object.tmd = saved->p[i].tmd;
                    mad->object[i]->locate.coord.t[0] =
                        saved->p[i].x;
                    mad->object[i]->locate.coord.t[1] =
                        saved->p[i].y;
                    mad->object[i]->locate.coord.t[2] =
                        saved->p[i].z;
                    i++;
                } while (i < mad->n);
            }
            if (item->owner->status == STAT_SQUAT)
            {
                NowReturnNormal(item->owner);
            }
            HenshinItem = 0;
            ((volatile TItem *)item)->owner->itmctl = 0;
        }
        item->mode = 0;
        return;
    }

    switch (item->mode)
    {
    case 0:
        SetNowMotion(human, MOT_ITEM_KAENGEKI, 1);
        Sound(item->owner, 0x4c);
        item->mode++;
        return;

    case 1:
    {
        MotionManager *motion;

        motion = human->motion;
        if (motion->mid != MOT_ITEM_KAENGEKI)
        {
            VECTOR *pos;
            Humanoid *drop_owner;
            s32 itemID;

            pos = GetAbsolutePosition(item->locate, 0, 0, 0);
            drop_owner = item->owner;
            itemID = item->type;
            memset(&scratch.p, 0, sizeof(PARAM_ITEM_LAUNCH));
            scratch.p.type = itemID;
            scratch.p.user = drop_owner;
            scratch.p.start.vx = pos->vx;
            scratch.p.start.vy = pos->vy;
            scratch.p.start.vz = pos->vz;
            scratch.p.end.vx = rand() % 200 - 100;
            scratch.p.end.vy = rand() % 100 - 200;
            scratch.p.end.vz = rand() % 200 - 100;
            ReqItemDrop(&scratch.p);
            if (item->proc == 0)
            {
                return;
            }
            item->mode = ff;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
            return;
        }
        if (motion->count != 0)
        {
            return;
        }
        if (motion->loop == 0)
        {
            return;
        }

        NowReturnNormal(human);
        scratch.sv = svec_y_n50[0];
        SetSmoke((VECTOR *)mad->locate.coord.t, &scratch.sv, 10, 6);
        {
            TItem *old;

            old = HenshinItem;
            if (old != 0 && old->proc != 0)
            {
                old->mode = ff;
                old->proc(old);
                DeleteConflict(old->locate);
                if (old->mode != 0)
                {
                    AdtMessageBox(msg_item_dispose_fail, old->type, (u32)old->mode);
                }
                old->owner = 0;
                old->proc = 0;
            }
        }
        HenshinItem = item;
        item->mode++;
        return;
    }

    case 2:
    {
        s32 i;
        HenshinModelSnapshot *saved;
        volatile TItem *vitem;
        Humanoid *mode_owner;
        u16 itemID;

        i = 0;
        saved = &HenshinSnapshot;
        mad->rotate.pad = (s16)saved->waist;
        if (mad->n > 0)
        {
            do
            {
                mad->object[i]->object.tmd = saved->p[i].tmd;
                mad->object[i]->locate.coord.t[0] =
                    saved->p[i].x;
                mad->object[i]->locate.coord.t[1] =
                    saved->p[i].y;
                mad->object[i]->locate.coord.t[2] =
                    saved->p[i].z;
                i++;
            } while (i < mad->n);
        }
        vitem = item;
        vitem->mode++;
        HenshinCount = 600;
        mode_owner = vitem->owner;
        itemID = *(volatile u16 *)&vitem->type;
        EmergencyNotice = -600;
        mode_owner->itmctl = itemID;
        return;
    }

    case 3:
    {
        u16 count;

        count = HenshinCount - 1;
        HenshinCount = count;
        if ((s32)(count << 16) > 0 &&
            item->owner->itmctl == item->type &&
            item->owner->status != STAT_DAMAGE &&
            item->owner->status != STAT_DEAD)
        {
            if (item->owner->status != STAT_ATTACK)
            {
                return;
            }
            if (item->owner->motion->loop >= 0 &&
                item->owner->motion->mid < MOT_ATTACK_STEALTH_BACK)
            {
                return;
            }
        }
        scratch.sv = svec_y_n50[0];
        SetSmoke((VECTOR *)mad->locate.coord.t, &scratch.sv, 10, 6);
        if (item->proc == 0)
        {
            return;
        }
        item->mode = ff;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != 0)
        {
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        }
        item->owner = 0;
        item->proc = 0;
        return;
    }
    }
}

