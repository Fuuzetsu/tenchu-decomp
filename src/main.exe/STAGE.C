#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "humanoid.h"
#include "item.h"
#include "model.h"
#include "score.h"
#include "stage.h"
#include "vmemory.h"
#include <psxsdk/libgpu.h>

/*
 * Demo STAGE.C defines SetupStageSequence before StartStageSequence. Retail
 * moved it behind the newly added score calculation; all four demo routines
 * are externally linked, so this is a source revision rather than deferred
 * inline emission.
 */

extern s32 StageTime;
extern s32 AttackActionCount;
extern u8 STAGE_LAYOUT_NUMBER;
extern ScoreResult STAGE_SCORE_COMPONENTS;

extern char fmt_dbg_quad[];    /* %d-%d-%d-%d  */
extern char fmt_dbg_counts[];  /* %d/%d/%d(%d/%d)  */
extern char fmt_num_paren[];   /* %d(%d) */
extern char fmt_num_bracket[]; /*  [%d] */
extern char str_newline_3[];
extern char fmt_stage_esd[];   /* %sSTAGE%d.ESD */
extern char path_anim[];       /* K:\\WORK\\CDIMAGE\\ANIM\\ */

extern void UpdateEvent(s16 n, s16 id);
extern s16 CVAsequence(s16 sid);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void StartStageSequence(void);
 *     STAGE.C:93, 56 src lines, frame 48 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s2       struct StageCharType * stg
 *     reg   $s0       struct Humanoid * human
 *     reg   $v0       long y
 *     reg   $a1       short i
 *     reg   $a0       short tp
 *
 * Globals it touches, as the original declared them:
 *     extern struct StageCharType StageChar[18];
 *     extern struct Humanoid *HumanGroup[32];
 *     extern int StageID;
 *     extern struct Humanoid *StagePlayer;
 *     extern short Humans;
 *     extern unsigned long *GlobalAreaMap;
 *     extern short StageCitizens;
 *     extern short StageEnemies;
 *     extern struct EventSeqType *StageEvent;
 *     extern long GameClock;
 *     extern long StageTime;
 *     extern short ActionHalt;
 *     extern long AttackActionCount;
 *     extern short FriendHits;
 *     extern short Murders;
 *     extern short Findenemies;
 *     extern short Criticals;
 *     extern struct EventSeqType *Event[2];
 * END PSX.SYM */

void StartStageSequence(void)
{
    StageCharType *stg;
    Humanoid *human;
    Humanoid *entry;
    Humanoid *order[40];
    s32 y;
    s16 i;
    s16 tp;

    stg = StageChar;
    while (stg->stage != STAGE_CHAR_END)
    {
        if (stg->stage == STAGE_NUMBER(StageID))
        {
            enum
            {
                STAGE_CHAR_PARTNER = -1,
                STAGE_CHAR_STORY_NPC = -2
            };
            switch (stg->chrid)
            {
            case STAGE_CHAR_PARTNER:
                /* The partner ninja — whichever of the pair the player did
                 * not pick. */
                tp = RIKIMARU_1;
                if (StagePlayer->type == RIKIMARU_0)
                {
                    tp = AYAME_1;
                }
                break;

            case STAGE_CHAR_STORY_NPC:
                /* The player-specific story NPC — Rikimaru's stages place
                 * the lord, Ayame's the princess. */
                tp = HIME;
                if (StagePlayer->type == RIKIMARU_0)
                {
                    tp = TONO;
                }
                break;

            default:
                tp = stg->chrid;
                break;
            }

            i = 0;
            while (i < Humans)
            {
                if (HumanGroup[i]->type == tp)
                {
                    break;
                }
                i++;
            }
            if (i == Humans)
            {
                human = BreedLife(tp, 0, 0, 0, 0);
            }
            else
            {
                human = HumanGroup[i];
            }

            y = stg->position.vx * 1000;
            human->point[HUMANOID_HOME_X] = y;
            human->locate->vx = y;
            y = stg->position.vz * 1000;
            human->point[HUMANOID_HOME_Z] = y;
            human->locate->vz = y;
            human->locate->vy = stg->position.vy * 1000;
            y = GetAreaMapLevel(GlobalAreaMap, human->locate->vx,
                                human->locate->vy - 1000,
                                human->locate->vz, AREA_LEVEL_DEFAULT);
            if (y < human->locate->vy)
            {
                human->locate->vy = y;
            }
            human->rotate->vy = stg->position.pad;
            UpdateCoordinate((ModelType *)human->model);
            SetupThinkFunction(human, stg->think);
            {
                ModelArchiveType *target;

                target = StagePlayer->model;
                human->attribute = (human->attribute | ATTR_SUSPEND | PHASE_ALERT) & ~ATTR_CUSTOMAI;
                human->life = HUMANOID_LIFE_INACTIVE;
                human->target = &target->locate;
            }
        }
        stg++;
    }

    i = 0;
    tp = 0;
    while (i < Humans)
    {
        entry = HumanGroup[i];
        if (entry != 0 && (((u16)entry->type & PAGE_MASK) == PAGE_PALACE))
        {
            order[tp++] = entry;
            HumanGroup[i] = 0;
        }
        i++;
    }

    i = 0;
    while (i < Humans)
    {
        entry = HumanGroup[i];
        if (entry != 0 && (((u16)entry->type & PAGE_MASK) == PAGE_BOSS))
        {
            order[tp++] = entry;
            HumanGroup[i] = 0;
        }
        i++;
    }

    for (i = 0; i < Humans; i++)
    {
        entry = HumanGroup[i];
        if (entry != 0)
        {
            order[tp++] = entry;
        }
    }

    i = 0;
    while (i < Humans)
    {
        HumanGroup[i] = order[i];
        i++;
    }

    StageCitizens = 0;
    StageEnemies = 0;
    StageBosses = 0;
    for (i = 0; i < Humans; i++)
    {
        u16 kind;

        human = HumanGroup[i];
        if (human->lifemax < 0)
        {
            continue;
        }
        if (human == StagePlayer)
        {
            continue;
        }
        if (human->type == NINKEN)
        {
            continue;
        }
        kind = (u16)human->type & PAGE_MASK;
        if (kind == PAGE_BOSS)
        {
            StageBosses++;
        }
        else if (kind == PAGE_CIVILIAN)
        {
            StageCitizens++;
            continue;
        }
        StageEnemies++;
    }

    switch (StageID)
    {
    case STAGE_ID_CORRUPT_MINISTER:
        StageBosses--;
        StageEnemies--;
        if (StagePlayer->type == AYAME_0)
            break;
        /* fallthrough */
    case STAGE_ID_CAPTIVE_NINJA:
    case STAGE_ID_MANJI_CULT:
        StageBosses--;
        StageEnemies--;
        break;
    }

    GameClock = 0;
    StageTime = 0;
    ActionHalt = ACTION_HALT_NONE;
    AttackActionCount = 0;
    FriendHits = 0;
    Murders = 0;
    Findenemies = 0;
    Criticals = 0;
    if (StageEvent != 0)
    {
        UpdateEvent(0, 0);
        Event[1] = 0;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short StageSequence(void);
 *     STAGE.C:153, 92 src lines, frame 56 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       struct EventSeqType * ev
 *     reg   $s0       struct Humanoid * tgt
 *     reg   $s3       short i
 *     reg   $s2       short flag
 *     reg   $s0       long gc
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern struct EventSeqType *Event[2];
 *     extern long StageTime;
 *     extern struct TCameraStatus CamState;
 *     extern short ActionHalt;
 *     extern enum TSystemFlag SystemFlag;
 *     extern short SkipFrame;
 *     extern long GameClock;
 *     extern int StageID;
 *     extern long EmergencyNotice;
 *     extern short Findenemies;
 *     extern short Murders;
 *     extern short Criticals;
 *     extern short StageEnemies;
 *     extern short FriendHits;
 *     extern short StageCitizens;
 *     extern struct Humanoid *eTarget[2];
 * END PSX.SYM */

s32 StageSequence(void)
{
    EventSeqType *ev;
    Humanoid *tgt;
    s16 i;
    s16 flag;
    s32 gc;
    s32 d;
    u16 sid;
    ScoreStats score_stats;

    flag = 0;
    if (StagePlayer->status == STAT_DEAD)
    {
        s32 result;

        if (StagePlayer->motion->loop == 0 &&
            StagePlayer->motion->count < 30)
        {
            return 0;
        }
        if (Event[0] == 0 && Event[1] == 0)
        {
            if (StagePlayer->motion->loop != 0)
            {
                StageTime++;
            }
            result = 0;
            if (StageTime >= 0)
            {
                result = -1;
            }
            return result;
        }
        /* Boot the two master scripts that keep running through player death. */
        UpdateEvent(0, EVENT_ROOT_FIRST);
        UpdateEvent(1, EVENT_ROOT_LAST);
        StagePlayer->status = STAT_ACTION;
        if ((s16)StageSequence() != 0)
        {
            return -1;
        }
        StageTime = -100;
        Event[1] = 0;
        Event[0] = 0;
        if (CamState.Mode != CMODE_FALL)
        {
            SetCameraMode(CMODE_CRITICAL_HIT);
        }
        ActionHalt = ACTION_HALT_STAGE_END;
        StagePlayer->status = STAT_DEAD;
        return 0;
    }

    if ((SystemFlag & SYSFLAG_DEBUGMODE) != 0 && SkipFrame == 0)
    {
        FntPrint(fmt_dbg_quad, STAGE_NUMBER(StageID), STAGE_LAYOUT_NUMBER,
                 GameClock / 30, EmergencyNotice);
        FntPrint(fmt_dbg_counts, Findenemies, Murders, Criticals,
                 StageEnemies, StageBosses);
        FntPrint(fmt_num_paren, FriendHits, StageCitizens);
        if (Event[0] != 0)
        {
            FntPrint(fmt_num_bracket, Event[0]->header.route.id);
        }
        if (Event[1] != 0)
        {
            FntPrint(fmt_num_bracket, Event[1]->header.route.id);
        }
        FntPrint(str_newline_3);
    }

    StageTime++;
    for (i = 0; i < N_STAGE_EVENT_SLOTS; i++)
    {
        ev = Event[i];
        if (ev == 0)
        {
            continue;
        }
        if ((u8)(ev->header.route.id - EVENT_ROOT_FIRST) >=
                N_STAGE_EVENT_SLOTS &&
            StagePlayer->life == 0)
        {
            continue;
        }

        tgt = eTarget[i];
        switch (ev->mode)
        {
        case EVTRIG_ALWAYS:
            flag = 1;
            break;

        case EVTRIG_ZONE:
            if ((u8)(ev->header.route.id - EVENT_ROOT_FIRST) <
                N_STAGE_EVENT_SLOTS)
            {
                tgt = StagePlayer;
            }
            d = (s16)(tgt->locate->vx / 1000);
            if (d < ev->trigger.zone.x.min ||
                ev->trigger.zone.x.max < d)
            {
                break;
            }
            d = (s16)(tgt->locate->vz / 1000);
            if (d < ev->trigger.zone.z.min ||
                ev->trigger.zone.z.max < d)
            {
                break;
            }
            d = (s16)(tgt->locate->vy / 1000);
            if (d < ev->trigger.zone.y.min ||
                ev->trigger.zone.y.max < d)
            {
                break;
            }
            flag = 1;
            break;

        case EVTRIG_ATTRIBUTE:
            if (((u16)tgt->attribute &
                 (u16)ev->trigger.attribute_mask) ==
                (u16)ev->trigger.attribute_mask)
            {
                flag = 1;
            }
            break;

        case EVTRIG_STATUS:
            if (StagePlayer->status != STAT_ATTACK &&
                tgt->status == ev->trigger.status)
            {
                flag = 1;
            }
            break;

        case EVTRIG_MOTION:
            if (tgt->motion->mid == ev->trigger.motion)
            {
                flag = 1;
            }
            break;

        case EVTRIG_LIFE:
            if (tgt->life <= ev->trigger.life)
            {
                flag = 1;
            }
            break;

        case EVTRIG_NEAR:
        {
            VECTOR *player_pos;
            VECTOR *target_pos;

            player_pos = StagePlayer->locate;
            target_pos = tgt->locate;
            if (__builtin_abs(player_pos->vy - target_pos->vy) > 2000)
            {
                break;
            }
            if (__builtin_abs(player_pos->vx - target_pos->vx) > 2000)
            {
                break;
            }
            if (__builtin_abs(player_pos->vz - target_pos->vz) <= 2000)
            {
                flag = 1;
            }
            break;
        }

        case EVTRIG_TIME:
            if (ev->trigger.time >= 0 && StageTime >= ev->trigger.time)
            {
                flag = 1;
            }
            break;

        case EVTRIG_MUSIC:
            flag = 1;
            PlayMusicFormID(ev->trigger.music);
            break;
        }

        if (flag != 0)
        {
            gc = GameClock;
            flag = 0;
            if ((u8)(ev->header.route.id - EVENT_ROOT_FIRST) >=
                    N_STAGE_EVENT_SLOTS &&
                StagePlayer->life == 0)
            {
                Event[i] = 0;
                continue;
            }
            /* EVENT_CVA_NONE means "no movie": fire the event
             * directly without a CVA sequence. */
            if (ev->header.route.event != EVENT_CVA_NONE)
            {
                sid = ev->header.route.event;
                if (sid == 0 && StageID == STAGE_ID_TRAINING)
                {
                    ScoreResult *score;

                    score = calculate_score(init_score_stats(&score_stats),
                                            (s16)StageID);
                    sid = 5 - (u16)score->grade;
                }
                if (CVAsequence((s16)sid) == 0)
                {
                    Event[i] = 0;
                    continue;
                }
            }
            /* Rikimaru watching Hikone's life hit zero reroutes the
             * follow-up to event 100 (his version of the finale). */
            if (StagePlayer->type == RIKIMARU_0 && ev->mode == EVTRIG_LIFE &&
                tgt->type == HIKONE)
            {
                ev->header.route.next1 = EVENT_RIKIMARU_FINALE;
            }
            StageTime = 0;
            GameClock = gc;
            UpdateEvent(0, ev->header.route.next1);
            UpdateEvent(1, ev->header.route.next2);
            if ((u8)(ev->header.route.id - 1) < 3)
            {
                return 1;
            }
            return (s16)StageSequence();
        }
    }
    return 0;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateEvent(short n, short id);
 *     STAGE.C:249, 18 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short n
 *     param $a1       short id
 *     reg   $a3       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct EventSeqType *Event[2];
 *     extern struct EventSeqType *StageEvent;
 *     extern struct Humanoid *eTarget[2];
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

void UpdateEvent(short n, short id)
{
    short i;

    Event[n] = 0;
    if (id == EVENT_ID_NONE)
        return;
    i = 0;
    if (StageEvent[0].header.word == EVENT_TABLE_END)
        return;

    do
    {
        if (StageEvent[i].header.route.id == id)
        {
            Event[n] = &StageEvent[i];
            if (StageEvent[i].target == EVENT_TARGET_PLAYER)
            {
                eTarget[n] = StagePlayer;
            }
            else
            {
                eTarget[n] = GetHumanoid(StageEvent[i].target);
            }
            {
                Humanoid *target;

                target = eTarget[n];
                if (target == 0 ||
                    (target->status == STAT_DEAD &&
                     target->motion->loop == MOTION_LOOP_DISABLED))
                {
                    Event[n] = 0;
                    return;
                }
            }
            if ((u16)(id - EVENT_ROOT_FIRST) >= N_STAGE_EVENT_SLOTS ||
                eTarget[n]->life > 0)
            {
                return;
            }
            Event[n] = 0;
            return;
        }
        i++;
    } while (StageEvent[i].header.word != EVENT_TABLE_END);
}

ScoreResult *calculate_score(ScoreStats *stats, packed_stage_id stage)
{
    ScoreResult *result;
    s32 score;
    s32 stealth_base;
    u8 spots;

    STAGE_SCORE_COMPONENTS.criticalScore = stats->criticals * SCORE_PER_CRITICAL;
    STAGE_SCORE_COMPONENTS.murderScore = stats->murders * SCORE_PER_MURDER;
    STAGE_SCORE_COMPONENTS.friendPenalty = stats->friendHits * SCORE_PER_FRIEND_HIT;

    spots = stats->findEnemies;
    stealth_base = STEALTH_BASE;

    if (spots != 0)
    {
        stealth_base = STEALTH_BASE_SEEN;
    }
    /* Training doubles the per-spot penalty.  `stage` is a StageConfig id,
     * not the campaign uid whose value 8 names Cure the Princess. */
    if (stage == STAGE_ID_TRAINING)
    {
        STAGE_SCORE_COMPONENTS.spottedScore =
            stealth_base - spots * (SPOT_PENALTY * 2);
    }
    else
    {
        STAGE_SCORE_COMPONENTS.spottedScore = stealth_base - spots * SPOT_PENALTY;
    }
    result = &STAGE_SCORE_COMPONENTS;
    if (result->spottedScore < 0)
    {
        result->spottedScore = 0;
    }

    result->score = STAGE_SCORE_COMPONENTS.criticalScore + result->murderScore +
                    result->friendPenalty + (u16)result->spottedScore;
    score = result->score;
    if (score < 0)
    {
        score = 0;
    }
    result->grade = score / SCORE_PER_GRADE;
    if (result->grade > RANK_GRAND_MASTER)
    {
        result->grade = RANK_GRAND_MASTER;
    }

    if (stats->stageBosses + stats->stageEnemies == 0)
    {
        memset(result, 0, sizeof(*result));
    }
    return result;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupStageSequence(void);
 *     STAGE.C:77, 12 src lines, frame 80 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     unsigned char [50] name
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *HumanGroup[32];
 *     extern struct EventSeqType *StageEvent;
 *     extern struct Humanoid *StagePlayer;
 *     extern int StageID;
 * END PSX.SYM */

void SetupStageSequence(void)
{
    u8 name[50];

    StagePlayer = HumanGroup[0];
    if (StageEvent != 0)
    {
        vfree(StageEvent);
    }
    sprintf((char *)name, fmt_stage_esd, path_anim,
            STAGE_NUMBER(StageID));
    StageEvent = (EventSeqType *)FileRead(name);
    StartStageSequence();
}

ScoreStats *init_score_stats(ScoreStats *stats)
{
    s32 clock;

    stats->stageBosses = (u8)StageBosses;
    stats->stageEnemies = (u8)StageEnemies;
    stats->findEnemies = (u8)Findenemies;
    stats->murders = (u8)Murders;
    stats->criticals = (u8)Criticals;
    stats->friendHits = (u8)FriendHits;
    clock = GameClock;
    if (clock > SCORE_CLOCK_MAX)
    {
        clock = SCORE_CLOCK_MAX;
    }
    stats->clock = clock;
    return stats;
}
