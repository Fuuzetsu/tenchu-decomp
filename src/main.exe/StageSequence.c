#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short StageSequence(void);
 *     STAGE.C:153, 92 src lines, frame 56 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     reg   $s1       struct EventSeqType * ev
 *     reg   $s0       struct Humanoid * tgt
 *     reg   $s3       short i
 *     reg   $s2       short flag
 *     reg   $s0       long gc
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern long StageTime;
 *     extern short ActionHalt;
 *     extern enum TSystemFlag SystemFlag;
 *     extern short SkipFrame;
 *     extern long GameClock;
 *     extern int StageID;
 *     extern short Findenemies;
 *     extern short Murders;
 *     extern short Criticals;
 *     extern short StageEnemies;
 *     extern short FriendHits;
 * END PSX.SYM */

/*
 * StageSequence (0x8004df58, 0x6cc bytes) advances the two active
 * stage-event slots, runs their mode-specific completion tests, starts the
 * selected movie/event, and updates the stage score/debug counters.
 *
 * Matching notes:
 *  - The switch is ordinary C.  Its compiler-generated nine-entry jump table
 *    exactly replaces the old split scaffold's StageSequence_jtbl at
 *    0x80012858; no hand-authored table or split-function labels remain.
 *  - The retail definition needs an `s32` return type even though the demo
 *    symbol records `short`.  All values are -1/0/1 and callers consume a
 *    short; explicit `(s16)` conversions at the two recursive uses reproduce
 *    retail's caller-side widening.  Defining this TU's function as `s16`
 *    adds a final sll/sra to the no-event result, one instruction absent from
 *    retail, so this is a measured retail/demo declaration divergence.
 *  - `StageTime` is volatile here so its -100 store remains before the camera
 *    state read.  The camera read is `CamState.mode`, not a standalone scalar:
 *    retaining the structure base is what gives retail's separate v0 address
 *    and v1 value registers for the lui/lw pair.
 *  - The mode-6 distance checks are inline `__builtin_abs` expressions.  A
 *    shared `d` temporary creates a persistent v0/v1 allocation tie; consuming
 *    each subtraction directly reproduces all three retail abs sequences.
 *  - The three debug-format addresses at 0x80097c7c/84/8c are fixed symbols,
 *    which preserves their exact lui/addiu construction instead of folding
 *    them through a shared base.
 */

typedef struct EventSeqType
{
    u8 id;
    u8 event;
    u8 next1;
    u8 next2;
    u8 target;
    u8 mode;
    u16 status;
    s16 x[2];
    s16 y[2];
    s16 z[2];
} EventSeqType;

typedef struct
{
    u8 bosses;
    u8 enemies;
    u8 hidden_finds;
    u8 murders;
    u8 criticals;
    u8 friendly_hits;
    u8 pad[2];
    s32 clock;
} ScoreStats;

typedef struct
{
    u16 value[6];
} ScoreResult;

typedef struct
{
    VECTOR target_vector;
    Humanoid *owner;
    s32 mode;
    s16 direction_rx;
    s16 direction_ry;
    s32 old_mode;
    u8 variation;
} CameraStatus;

extern Humanoid *StagePlayer;
extern EventSeqType *D_80097F78[2];
extern Humanoid *D_80097F80[2];
extern volatile s32 StageTime;
extern s32 GameClock;
extern s32 SystemFlag;
extern s32 StageID;
extern s32 FRAMES_UNTIL_END_OF_ALERT;
extern CameraStatus CamState;
extern s16 ActionHalt;
extern s16 SkipFrame;
extern s16 Findenemies;
extern s16 Murders;
extern s16 Criticals;
extern s16 StageEnemies;
extern s16 StageBosses;
extern s16 FriendHits;
extern s16 StageCitizens;
extern u8 STAGE_LAYOUT_NUMBER;
extern char D_80012830[];
extern char D_80012840[];
extern char D_80097C7C[];
extern char D_80097C84[];
extern char D_80097C8C[];

extern void UpdateEvent(s16 n, s16 id);
extern void SetCameraMode(s32 mode);
extern void FntPrint(char *format, ...);
extern void PlayMusicFormID(s32 id);
extern s16 CVAsequence(s16 sid);
extern ScoreStats *init_score_stats(ScoreStats *stats);
extern ScoreResult *calculate_score(ScoreStats *stats, s16 stage);

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
    if (StagePlayer->status == 0x11)
    {
        s32 result;

        if (StagePlayer->motion->loop == 0 &&
            StagePlayer->motion->count < 30)
        {
            return 0;
        }
        if (D_80097F78[0] != 0)
        {
            goto active_events;
        }
        if (D_80097F78[1] != 0)
        {
            goto active_events;
        }
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

active_events:
        UpdateEvent(0, 2);
        UpdateEvent(1, 3);
        StagePlayer->status = 1;
        if ((s16)StageSequence() != 0)
        {
            return -1;
        }
        StageTime = -100;
        D_80097F78[1] = 0;
        D_80097F78[0] = 0;
        if (CamState.mode != 14)
        {
            SetCameraMode(4);
        }
        ActionHalt = -1;
        StagePlayer->status = 0x11;
        return 0;
    }

    if ((SystemFlag & 2) != 0 && SkipFrame == 0)
    {
        FntPrint(D_80012830, StageID + 1, STAGE_LAYOUT_NUMBER,
                 GameClock / 30, FRAMES_UNTIL_END_OF_ALERT);
        FntPrint(D_80012840, Findenemies, Murders, Criticals,
                 StageEnemies, StageBosses);
        FntPrint(D_80097C7C, FriendHits, StageCitizens);
        if (D_80097F78[0] != 0)
        {
            FntPrint(D_80097C84, D_80097F78[0]->id);
        }
        if (D_80097F78[1] != 0)
        {
            FntPrint(D_80097C84, D_80097F78[1]->id);
        }
        FntPrint(D_80097C8C);
    }

    StageTime++;
    for (i = 0; i < 2; i++)
    {
        ev = D_80097F78[i];
        if (ev == 0)
        {
            continue;
        }
        if ((u8)(ev->id - 2) >= 2 && StagePlayer->life == 0)
        {
            continue;
        }

        tgt = D_80097F80[i];
        switch (ev->mode)
        {
        case 0:
            flag = 1;
            break;

        case 1:
            if ((u8)(ev->id - 2) < 2)
            {
                tgt = StagePlayer;
            }
            d = (s16)(tgt->locate->vx / 1000);
            if (d < ev->x[0] || ev->x[1] < d)
            {
                break;
            }
            d = (s16)(tgt->locate->vz / 1000);
            if (d < ev->z[0] || ev->z[1] < d)
            {
                break;
            }
            d = (s16)(tgt->locate->vy / 1000);
            if (d < ev->y[0] || ev->y[1] < d)
            {
                break;
            }
            flag = 1;
            break;

        case 2:
            if (((u16)tgt->attribute & ev->status) == ev->status)
            {
                flag = 1;
            }
            break;

        case 3:
            if (StagePlayer->status != 7 && tgt->status == (s16)ev->status)
            {
                flag = 1;
            }
            break;

        case 4:
            if (tgt->motion->mid == (s16)ev->status)
            {
                flag = 1;
            }
            break;

        case 5:
            if (tgt->life <= (s16)ev->status)
            {
                flag = 1;
            }
            break;

        case 6:
        {
            VECTOR *player_pos;
            VECTOR *target_pos;

            player_pos = StagePlayer->locate;
            target_pos = tgt->locate;
            if (__builtin_abs(player_pos->vy - target_pos->vy) >= 2001)
            {
                break;
            }
            if (__builtin_abs(player_pos->vx - target_pos->vx) >= 2001)
            {
                break;
            }
            if (__builtin_abs(player_pos->vz - target_pos->vz) < 2001)
            {
                flag = 1;
            }
            break;
        }

        case 7:
            if ((s16)ev->status >= 0 && StageTime >= (s16)ev->status)
            {
                flag = 1;
            }
            break;

        case 8:
            flag = 1;
            PlayMusicFormID((s16)ev->status);
            break;
        }

        if (flag != 0)
        {
            gc = GameClock;
            flag = 0;
            if ((u8)(ev->id - 2) < 2 || StagePlayer->life != 0)
            {
                if (ev->event == 0xff)
                {
                    goto run_event;
                }
                sid = ev->event;
                if (sid == 0 && StageID == 8)
                {
                    ScoreResult *score;

                    score = calculate_score(init_score_stats(&score_stats),
                                            (s16)StageID);
                    sid = 5 - score->value[5];
                }
                if (CVAsequence((s16)sid) != 0)
                {
                    goto run_event;
                }
            }
            D_80097F78[i] = 0;
            continue;

run_event:
            if (StagePlayer->type == 0 && ev->mode == 5 &&
                tgt->type == 0x8d)
            {
                ev->next1 = 100;
            }
            StageTime = 0;
            GameClock = gc;
            UpdateEvent(0, ev->next1);
            UpdateEvent(1, ev->next2);
            if ((u8)(ev->id - 1) < 3)
            {
                return 1;
            }
            return (s16)StageSequence();
        }
    }
    return 0;
}
