#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "score.h"
#include "stage.h"
#include <psxsdk/libgpu.h>

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

extern s32 StageTime;
extern long EmergencyNotice;
extern u8 STAGE_LAYOUT_NUMBER;
extern char fmt_dbg_quad[];    /* %d-%d-%d-%d  */
extern char fmt_dbg_counts[];  /* %d/%d/%d(%d/%d)  */
extern char fmt_num_paren[];   /* %d(%d) */
extern char fmt_num_bracket[]; /*  [%d] */
extern char str_newline_3[];

extern void UpdateEvent(s16 n, s16 id);
extern s16 CVAsequence(s16 sid);

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
