#include "common.h"
#include "main.exe.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemHenshin(struct tag_TItem *item);
 *     ITEM.C:2060, 140 src lines, frame 208 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * HenshinSnapshot, swaps in the disguise character for the HenshinCount
 * countdown with a smoke puff at both ends, ticks the countdown each frame,
 * and restores the original model when the timer runs out, damage breaks
 * the disguise, or the item is disposed (HenshinItem marks the active
 * instance).
 */
#include "item.h"

typedef union
{
    PARAM_ITEM_LAUNCH drop_request;
    SVECTOR smoke_velocity;
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
 *    path stores the current-disguise pointer before reloading item->owner.human,
 *    and HENSHIN_MODE_TRANSFORM finishes its mode/count stores before loading
 *    owner/type.
 *    The old disguise pointer is copied once before its null/proc checks so
 *    volatility does not introduce redundant global reloads.
 *  - `scratch` is the exact sp+0x10..0x37 lifetime overlay: PSX.SYM records
 *    a PARAM_ITEM_LAUNCH `drop_request` on the interrupted-motion path and
 *    an SVECTOR `smoke_velocity` on the smoke paths.
 *  - HENSHIN_MODE_START deliberately does not assign HenshinItem. It jumps
 *    directly to the shared mode increment; only the completed wait path
 *    installs the current item after disposing any prior disguise.
 */
extern TItem *volatile HenshinItem;
extern volatile u16 HenshinCount;
extern SVECTOR svec_y_n50[]; /* {0,-50,0} */

void ProcItemHenshin(TItem *item)
{
    enum
    {
        HENSHIN_MODE_START = 0,
        HENSHIN_MODE_WAIT = 1,
        HENSHIN_MODE_TRANSFORM = 2,
        HENSHIN_MODE_ACTIVE = 3,
        HENSHIN_DURATION = 600
    };
    Humanoid *human;
    ModelArchiveType *archive;
    ProcItemHenshinScratch scratch;

    human = item->owner.human;
    archive = human->model;

    if (item->mode == ITEM_MODE_DISPOSE)
    {
        if (item == HenshinItem)
        {
            s32 part_index;
            HenshinModelSnapshot *snapshot;

            part_index = 0;
            snapshot = &Item_save;
            archive->rotate.pad = (s16)snapshot->waist;
            if (archive->n > 0)
            {
                do
                {
                    archive->object[part_index]->object.tmd =
                        snapshot->p[part_index].tmd;
                    archive->object[part_index]->locate.coord.t[0] =
                        snapshot->p[part_index].x;
                    archive->object[part_index]->locate.coord.t[1] =
                        snapshot->p[part_index].y;
                    archive->object[part_index]->locate.coord.t[2] =
                        snapshot->p[part_index].z;
                    part_index++;
                } while (part_index < archive->n);
            }
            if (item->owner.human->status == STAT_SQUAT)
            {
                NowReturnNormal(item->owner.human);
            }
            HenshinItem = 0;
            ((volatile TItem *)item)->owner.human->itmctl = 0;
        }
        item->mode = HENSHIN_MODE_START;
        return;
    }

    switch (item->mode)
    {
    case HENSHIN_MODE_START:
        SetNowMotion(human, MOT_ITEM_KAENGEKI, 1);
        Sound(item->owner.human, SE_ITEM_USE);
        item->mode++;
        return;

    case HENSHIN_MODE_WAIT:
    {
        MotionManager *motion;

        motion = human->motion;
        if (motion->mid != MOT_ITEM_KAENGEKI)
        {
            VECTOR *drop_position;
            Humanoid *drop_owner;
            s32 itemID;

            drop_position = GetAbsolutePosition(item->locate, 0, 0, 0);
            drop_owner = item->owner.human;
            itemID = item->type;
            memset(&scratch.drop_request, 0, sizeof(PARAM_ITEM_LAUNCH));
            scratch.drop_request.type = itemID;
            scratch.drop_request.user.human = drop_owner;
            scratch.drop_request.start.vx = drop_position->vx;
            scratch.drop_request.start.vy = drop_position->vy;
            scratch.drop_request.start.vz = drop_position->vz;
            scratch.drop_request.end.vx = rand() % 200 - 100;
            scratch.drop_request.end.vy = rand() % 100 - 200;
            scratch.drop_request.end.vz = rand() % 200 - 100;
            ReqItemDrop(&scratch.drop_request);
            if (item->proc == 0)
            {
                return;
            }
            item->mode = ITEM_MODE_DISPOSE;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != HENSHIN_MODE_START)
            {
                AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
            }
            item->owner.human = 0;
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
        scratch.smoke_velocity = svec_y_n50[0];
        SetSmoke((VECTOR *)archive->locate.coord.t,
                 &scratch.smoke_velocity, 10, 6);
        {
            TItem *previous_disguise;

            previous_disguise = HenshinItem;
            if (previous_disguise != 0 && previous_disguise->proc != 0)
            {
                previous_disguise->mode = ITEM_MODE_DISPOSE;
                previous_disguise->proc(previous_disguise);
                DeleteConflict(previous_disguise->locate);
                if (previous_disguise->mode != HENSHIN_MODE_START)
                {
                    AdtMessageBox(msg_item_dispose_fail,
                                  previous_disguise->type,
                                  (u32)previous_disguise->mode);
                }
                previous_disguise->owner.human = 0;
                previous_disguise->proc = 0;
            }
        }
        HenshinItem = item;
        item->mode++;
        return;
    }

    case HENSHIN_MODE_TRANSFORM:
    {
        s32 part_index;
        HenshinModelSnapshot *snapshot;
        volatile TItem *volatile_item;
        Humanoid *disguise_owner;
        u16 itemID;

        part_index = 0;
        snapshot = &HenshinSnapshot;
        archive->rotate.pad = (s16)snapshot->waist;
        if (archive->n > 0)
        {
            do
            {
                archive->object[part_index]->object.tmd =
                    snapshot->p[part_index].tmd;
                archive->object[part_index]->locate.coord.t[0] =
                    snapshot->p[part_index].x;
                archive->object[part_index]->locate.coord.t[1] =
                    snapshot->p[part_index].y;
                archive->object[part_index]->locate.coord.t[2] =
                    snapshot->p[part_index].z;
                part_index++;
            } while (part_index < archive->n);
        }
        volatile_item = item;
        volatile_item->mode++;
        HenshinCount = HENSHIN_DURATION;
        disguise_owner = volatile_item->owner.human;
        /* TItemType is a 32-bit enum; retail deliberately reads its low
         * half. */
        itemID = *(volatile u16 *)&volatile_item->type;
        EmergencyNotice = -HENSHIN_DURATION;
        disguise_owner->itmctl = itemID;
        return;
    }

    case HENSHIN_MODE_ACTIVE:
    {
        u16 remaining_count;

        remaining_count = HenshinCount - 1;
        HenshinCount = remaining_count;
        if ((s16)remaining_count > 0 &&
            item->owner.human->itmctl == item->type &&
            item->owner.human->status != STAT_DAMAGE &&
            item->owner.human->status != STAT_DEAD)
        {
            if (item->owner.human->status != STAT_ATTACK)
            {
                return;
            }
            if (item->owner.human->motion->loop >= 0 &&
                item->owner.human->motion->mid < MOT_ATTACK_STEALTH_BACK)
            {
                return;
            }
        }
        scratch.smoke_velocity = svec_y_n50[0];
        SetSmoke((VECTOR *)archive->locate.coord.t,
                 &scratch.smoke_velocity, 10, 6);
        if (item->proc == 0)
        {
            return;
        }
        item->mode = ITEM_MODE_DISPOSE;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != HENSHIN_MODE_START)
        {
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        }
        item->owner.human = 0;
        item->proc = 0;
        return;
    }
    }
}
