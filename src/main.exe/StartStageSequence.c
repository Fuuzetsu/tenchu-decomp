#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "stage.h"

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

#include "item.h"

/*
 * MATCH.
 *
 * StartStageSequence (0x8004d970) installs the stage-specific characters,
 * reorders HumanGroup by character class, counts enemies/citizens/bosses,
 * applies the stage-specific score exclusions, and resets the event and
 * score clocks.
 *
 * Matching notes:
 *  - `order[40]` is the exact sp+0x18..sp+0xb7 reorder buffer; the outgoing
 *    fifth argument remains at sp+0x10 and the saved area starts at sp+0xb8.
 *  - The chrid translation is a two-case switch. expand_case keeps the
 *    STAGE_CHAR_PARTNER (-1) arm inline and lays the STAGE_CHAR_STORY_NPC
 *    (-2) arm out later while emitting the target's -2/-1 test order. The
 *    named `stg_think` pointer keeps the compiler's
 *    derived think-field base live; the volatile row view prevents CSE with
 *    the preceding signed chrid load.
 *  - `y` deliberately carries each x/z product to both destination stores;
 *    repeating the multiplication expression makes GCC recompute it.  The
 *    StagePlayer model is likewise fetched before the attribute/life stores
 *    so the target's v1 chain can be interleaved with them.
 *  - Loop spelling is mixed intentionally.  The first two class-filter scans
 *    are `while` loops, but the remaining-pointer scan and score-count scan
 *    are `for` loops.  With the pinned cc1 those loop notes make reorg copy
 *    `i + 1` into two taken-branch delay slots and recompute it on the other
 *    path; the equivalent all-while spelling is two instructions short.
 *  - StageEvent/StagePlayer and the score counters are gp-relative in this
 *    translation unit; maspsxflags.py records that per-function list.
 *  - The stage exclusions are another sparse switch: FREE_PRINCESS performs
 *    its first decrement and conditionally falls through to the shared
 *    stage-2/3 decrement body.
 */
extern s32 StageTime;
extern s32 AttackActionCount;

extern void SetupThinkFunction(Humanoid *human, TThinkType type);
extern void UpdateEvent(s16 n, s16 id);

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
                StageCharThinkOffset = 0x0c,
                STAGE_CHAR_PARTNER = -1,
                STAGE_CHAR_STORY_NPC = -2
            };
            s16 chrid;
            volatile u16 *stg_think;

            chrid = (s16)stg->chrid;
            stg_think = (volatile u16 *)&stg->think;
            /* Walked back from stg_think, not read off stg: byte-required
             * (the target addresses chrid as lhu -10(stg_think); measured). */
            tp = ((volatile StageCharType *)((u8 *)stg_think -
                                             StageCharThinkOffset))
                     ->chrid;
            switch (chrid)
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
                human->target.archive = target;
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
    ActionHalt = 0;
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
