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

#include "item.h"

extern TItem *HenshinItem;
extern u16 HenshinCount;
extern SVECTOR svec_y_n50[]; /* {0,-50,0} */

enum henshin_mode
{
    HENSHIN_MODE_START = 0,
    HENSHIN_MODE_WAIT = 1,
    HENSHIN_MODE_TRANSFORM = 2,
    HENSHIN_MODE_ACTIVE = 3
};

enum
{
    HENSHIN_DURATION = 600
};

void ProcItemHenshin(TItem *item)
{
    ModelArchiveType *archive;
    PARAM_ITEM_LAUNCH drop_request;

    archive = item->owner->model;

    if (item->mode == ITEM_MODE_DISPOSE)
    {
        if (item == HenshinItem)
        {
            HenshinModelSnapshot *snapshot;

            snapshot = &Item_save;
            ApplyHenshinModel(snapshot, archive);
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
        SetNowMotion(item->owner, MOT_ITEM_KAENGEKI, MOTION_MOVE_APPLY);
        Sound(item->owner, SE_ITEM_USE);
        item->mode++;
        return;

    case HENSHIN_MODE_WAIT:
    {
        Humanoid *human;
        MotionManager *motion;

        human = item->owner;
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
        SetSmoke(MODEL_POSITION(archive),
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
        HenshinModelSnapshot *snapshot;
        Humanoid *disguise_owner;
        u16 itemID;

        snapshot = &HenshinSnapshot;
        ApplyHenshinModel(snapshot, archive);
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
        SetSmoke(MODEL_POSITION(archive),
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
