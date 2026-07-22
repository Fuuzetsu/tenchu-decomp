#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void StartStageSequence(void);
 *     STAGE.C:93, 56 src lines, frame 48 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 * END PSX.SYM */

#include "item.h"

/*
 * MATCH.
 *
 * StartStageSequence (0x8004d970) installs the stage-specific characters,
 * reorders HumanGroup by character class, counts enemies/citizens/bosses,
 * applies the stage-specific score exclusions, and resets the event and
 * score clocks.  INIT_STAGE_STATS is an internal symbol at the reset tail,
 * not a second function.
 *
 * Matching notes:
 *  - `order[40]` is the exact sp+0x18..sp+0xb7 reorder buffer; the outgoing
 *    fifth argument remains at sp+0x10 and the saved area starts at sp+0xb8.
 *  - The chrid translation is written as two explicit gotos so the -1 arm
 *    stays inline and the -2 arm is laid out later.  `stg_think` is a
 *    short-lived volatile view of the same StageChar row: it prevents CSE of
 *    the signed comparison load with the unsigned `tp` load, while its named
 *    pointer makes GCC reuse the derived `&stg->think` induction base (s1).
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
 */
extern StageCharType StageChar[18];
extern Humanoid *HumanGroup[32];
extern Humanoid *StagePlayer;
extern EventSeqType *StageEvent;
extern EventSeqType *D_80097F7C;
extern s32 StageID;
extern s32 GameClock;
extern s32 StageTime;
extern s32 AttackActionCount;
extern s16 Humans;
extern s16 ActionHalt;
extern u16 StageCitizens;
extern u16 StageEnemies;
extern u16 StageBosses;
extern u16 FriendHits;
extern u16 Murders;
extern u16 Findenemies;
extern u16 Criticals;

extern Humanoid *BreedLife(s16 type, s32 x, s32 y, s32 z, s32 r);
extern s32 GetAreaMapLevel(u_long *area, s32 x, s32 y, s32 z, s32 mode);
extern void UpdateCoordinate(ModelType *model);
extern void SetupThinkFunction(Humanoid *human, s16 type);
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
    while (stg->stage != -1)
    {
        if (stg->stage == StageID + 1)
        {
            s16 chrid;
            volatile u16 *stg_think;

            chrid = (s16)stg->chrid;
            stg_think = (volatile u16 *)&stg->think;
            tp = stg_think[-5];
            if (chrid == -2)
            {
                goto chrid_minus_two;
            }
            if (chrid != -1)
            {
                goto chrid_ready;
            }
            tp = 2;
            if (StagePlayer->type == 0)
            {
                tp = 3;
            }
            goto chrid_ready;

chrid_minus_two:
            tp = 5;
            if (StagePlayer->type == 0)
            {
                tp = 6;
            }

chrid_ready:

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
            human->point[0] = y;
            human->locate->vx = y;
            y = stg->position.vz * 1000;
            human->point[1] = y;
            human->locate->vz = y;
            human->locate->vy = stg->position.vy * 1000;
            y = GetAreaMapLevel(GlobalAreaMap, human->locate->vx,
                                human->locate->vy - 1000,
                                human->locate->vz, 0);
            if (y < human->locate->vy)
            {
                human->locate->vy = y;
            }
            human->rotate->vy = stg->position.pad;
            UpdateCoordinate((ModelType *)human->model);
            SetupThinkFunction(human, stg->think);
            {
                ModelType *target;

                target = (ModelType *)StagePlayer->model;
                human->attribute = (human->attribute | 0x82) & 0xfffb;
                human->life = -1;
                human->target = target;
            }
        }
        stg++;
    }

    i = 0;
    tp = i;
    while (i < Humans)
    {
        entry = HumanGroup[i];
        if (entry != 0 && (((u16)entry->type & 0xf0) == 0))
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
        if (entry != 0 && (((u16)entry->type & 0xf0) == 0x80))
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
        if ((s16)human->lifemax < 0)
        {
            goto next_human;
        }
        if (human == StagePlayer)
        {
            goto next_human;
        }
        if (human->type == 0xa9)
        {
            goto next_human;
        }
        kind = (u16)human->type & 0xf0;
        if (kind == 0x80)
        {
            StageBosses++;
        }
        else if (kind == 0x90)
        {
            StageCitizens++;
            goto next_human;
        }
        StageEnemies++;
next_human:
        ;
    }

    if (StageID >= 2)
    {
        if (StageID >= 4)
        {
            if (StageID != 10)
            {
                goto init_stats;
            }
            StageBosses--;
            StageEnemies--;
            if (StagePlayer->type == 1)
            {
                goto init_stats;
            }
        }
        StageBosses--;
        StageEnemies--;
    }

init_stats:
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
        D_80097F7C = 0;
    }
}
