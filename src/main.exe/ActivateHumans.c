#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActivateHumans(void);
 *     WORLD.C:752, 41 src lines, frame 48 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       int i
 *     reg   $s0       struct Humanoid * target
 *     stack sp+16     struct VECTOR vc
 *     reg   $s0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct Humanoid *StagePlayer;
 *     extern long GameClock;
 *     extern short SkipFrame;
 *     extern struct StageCharType StageChar[18];
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern int StageID;
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

extern s32 PacketUsed; /* This TU uses a signed view of the u32 definition. */
extern s16 ThinkBudgetRaw;
extern s16 ThinkCount;
extern s16 ThinkBudget;

/* One thinking human costs about this many GPU packet bytes, and the
 * budget keeps PacketUsed a whole unit clear of the end of a Packet[]
 * row: 0x10000 - THINK_PACKET_COST == 0xec78. */
#define THINK_PACKET_COST 5000
#define THINK_PACKET_LIMIT (0x10000 - THINK_PACKET_COST)

void ActivateHumans(void)
{
    s32 i;
    Humanoid *target;
    VECTOR vc;
    s32 activate_distance;

    target = CamState.Owner;
    vc = *target->locate;
    activate_distance = ACTIVATE_RADIUS_WIDE;
    if (StagePlayer->motion->mid != MOT_ITEM_SHINSOKU)
    {
        activate_distance = ACTIVATE_RADIUS;
    }

    if (GameClock % 30 != 0 || SkipFrame != 0)
    {
        return;
    }

    /* GPU-packet headroom: how many more humans may think this frame. */
    ThinkBudgetRaw = (THINK_PACKET_LIMIT - PacketUsed) / THINK_PACKET_COST - 1;
    ThinkBudget = ThinkBudgetRaw < 2
                      ? (u16)ThinkBudget - 1
                      : ThinkBudgetRaw;
    /* Clamp the usable budget to 3..6. */
    ThinkBudget = ThinkBudget < 7
                      ? (ThinkBudget < 3 ? 3 : ThinkBudget)
                      : 6;
    i = 0;
    ThinkCount = 0;
    while (1)
    {
        Humanoid *human;

        if ((s16)i >= Humans)
        {
            return;
        }
        human = HumanGroup[(s16)i];
        if (human != target)
        {
            s32 active;
            s32 final;
            s32 distance;
            s16 j;

            distance = GetVectorDistance(human->locate, &vc);
            if (distance > DEACTIVATE_RADIUS)
            {
                active = 0;
                goto active_done;
            }
            if (((u16)human->type & PAGE_MASK) == PAGE_BOSS)
            {
                active = 1;
                goto active_done;
            }
            if (human->type == NINKEN || human->life < 0)
            {
                active = 1;
                goto active_done;
            }
            if (GameClock == 30 || StageID == STAGE_ID_TRAINING)
            {
                active = 1;
                goto active_done;
            }
            if (VISIBLE_ENEMIES_ < ThinkBudget)
            {
                active = 1;
                if (ThinkCount < ThinkBudget)
                {
                    goto active_done;
                }
                final = distance < activate_distance;
                goto visible_done;
            }
            if (distance >= activate_distance)
            {
                active = 0;
                goto active_done;
            }
            if (((u16)human->attribute & ATTR_SUSPEND) == 0 &&
                ThinkCount < ThinkBudget)
            {
                active = 1;
                goto active_done;
            }
            j = 0;
            while (VISIBLE_CHARACTERS_ON_STAGE_[j] != human)
            {
                if (VISIBLE_ENEMIES_ <= j)
                {
                    break;
                }
                j++;
            }
            final = j != VISIBLE_ENEMIES_;

        visible_done:
            /* This earlier `final` lifetime ends at the visibility join; the
             * active-result join below overwrites it before its next use. */
            active = final;
        active_done:
            if (human)
            {
                final = 0;
                final = active;
            }
            else
            {
                final = active;
            }
            if (final)
            {
                if (((u16)human->attribute & ATTR_SUSPEND) == 0)
                {
                    ThinkCount++;
                }
                else if (StageID == STAGE_ID_TRAINING || human->life < 0 || GameClock == 30 ||
                         (ThinkCount < ThinkBudget && distance > ACTIVATE_RADIUS))
                {
                    human->attribute = (u16)human->attribute & ~ATTR_SUSPEND;
                    ThinkCount++;
                    (*human->model->object)->attribute |= MODEL_ATTR_COLLIDE;
                }
            }
            else if (((u16)human->attribute & ATTR_SUSPEND) == 0 && human->type != ON)
            {
                if ((human->type == NINJA_0 &&
                     (u32)(StageID - STAGE_ID_RECLAIM_CASTLE) <=
                         STAGE_ID_FREE_PRINCESS - STAGE_ID_RECLAIM_CASTLE) ||
                    human->type == GOO)
                {
                    j = 0;
                    while (StageChar[j].stage != STAGE_CHAR_END)
                    {
                        if (StageChar[j].stage == STAGE_NUMBER(StageID) &&
                            StageChar[j].chrid == human->type)
                        {
                            human->model->locate.coord.t[0] = StageChar[j].position.vx * 1000;
                            human->model->locate.coord.t[1] = StageChar[j].position.vy * 1000;
                            human->model->locate.coord.t[2] = StageChar[j].position.vz * 1000;
                        }
                        j++;
                    }
                    if (human->type == GOO && human->life == 0)
                    {
                        human->life = 1;
                    }
                }
                else if (human->status != STAT_DEAD && ((u16)human->attribute & ATTR_FLOAT) == 0)
                {
                    VECTOR query;
                    VECTOR work;
                    s32 level;

                    memset(&work, 0, sizeof(work));
                    work.vx = human->point[HUMANOID_HOME_X];
                    work.vy = human->locate->vy - 1500;
                    work.vz = human->point[HUMANOID_HOME_Z];
                    query = work;
                    if (GetVectorDistance(&query, &vc) > DEACTIVATE_RADIUS)
                    {
                        level = GetAreaMapLevel(GlobalAreaMap, query.vx, query.vy,
                                                query.vz, AREA_LEVEL_STEP_DOWN);
                        if (level != LEVEL_NONE)
                        {
                            human->model->locate.coord.t[0] = query.vx;
                            human->model->locate.coord.t[1] = level;
                            human->model->locate.coord.t[2] = query.vz;
                        }
                    }
                }

                human->attribute = (u16)human->attribute | ATTR_SUSPEND;
                (*human->model->object)->attribute &= ~MODEL_ATTR_COLLIDE;
            }
        }

        i++;
    }
}
