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
 *  - HenshinItem and HenshinCount are ordinary gameplay globals; their former
 *    volatile qualifiers were inert and have been removed. Two owner-slot
 *    reads still use narrow volatile views to retain retail's post-store
 *    reloads without qualifying the item itself. The old disguise pointer is
 *    copied once before its null/proc checks, avoiding redundant global reads.
 *  - `drop_request` occupies the exact sp+0x10..0x37 slot. The smoke paths
 *    reuse its leading bytes for their short velocity.
 *  - HENSHIN_MODE_START deliberately does not assign HenshinItem. It jumps
 *    directly to the shared mode increment; only the completed wait path
 *    installs the current item after disposing any prior disguise.
 */
extern TItem *HenshinItem;
extern u16 HenshinCount;
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
    PARAM_ITEM_LAUNCH drop_request;

    human = item->owner;
    archive = human->model;

    if (item->mode == ITEM_MODE_DISPOSE)
    {
        if (item == HenshinItem)
        {
            s32 part_index;
            HenshinModelSnapshot *snapshot;

            snapshot = &Item_save;
            APPLY_HENSHIN_MODEL(snapshot, archive, part_index);
            if (item->owner->status == STAT_SQUAT)
            {
                NowReturnNormal(item->owner);
            }
            HenshinItem = 0;
            (*(Humanoid *volatile *)&item->owner)->active_item =
                ACTIVE_ITEM_NONE;
        }
        item->mode = HENSHIN_MODE_START;
        return;
    }

    switch (item->mode)
    {
    case HENSHIN_MODE_START:
        SetNowMotion(human, MOT_ITEM_KAENGEKI, MOTION_MOVE_APPLY);
        Sound(item->owner, SE_ITEM_USE);
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
            drop_owner = item->owner;
            itemID = item->type;
            memset(&drop_request, 0, sizeof(PARAM_ITEM_LAUNCH));
            drop_request.type = itemID;
            drop_request.user = drop_owner;
            drop_request.start.vx = drop_position->vx;
            drop_request.start.vy = drop_position->vy;
            drop_request.start.vz = drop_position->vz;
            drop_request.end.vx = rand() % 200 - 100;
            drop_request.end.vy = rand() % 100 - 200;
            drop_request.end.vz = rand() % 200 - 100;
            ReqItemDrop(&drop_request);
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
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
        *(SVECTOR *)&drop_request = svec_y_n50[0];
        SetSmoke((VECTOR *)archive->locate.coord.t,
                 (SVECTOR *)&drop_request, 10, 6);
        {
            TItem *previous_disguise;

            previous_disguise = HenshinItem;
            if (previous_disguise != 0 && previous_disguise->proc != 0)
            {
                DISPOSE_ITEM(previous_disguise);
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
        Humanoid *disguise_owner;
        u16 itemID;

        snapshot = &HenshinSnapshot;
        APPLY_HENSHIN_MODEL(snapshot, archive, part_index);
        item->mode++;
        HenshinCount = HENSHIN_DURATION;
        disguise_owner = *(Humanoid *volatile *)&item->owner;
        /* TItemType is a 32-bit enum; retail deliberately reads its low
         * half. */
        itemID = *(u16 *)&item->type;
        EmergencyNotice = -HENSHIN_DURATION;
        disguise_owner->active_item = itemID;
        return;
    }

    case HENSHIN_MODE_ACTIVE:
    {
        u16 remaining_count;

        remaining_count = HenshinCount - 1;
        HenshinCount = remaining_count;
        if ((s16)remaining_count > 0 &&
            item->owner->active_item == item->type &&
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
        *(SVECTOR *)&drop_request = svec_y_n50[0];
        SetSmoke((VECTOR *)archive->locate.coord.t,
                 (SVECTOR *)&drop_request, 10, 6);
        if (item->proc == 0)
        {
            return;
        }
        DISPOSE_ITEM(item);
        return;
    }
    }
}
