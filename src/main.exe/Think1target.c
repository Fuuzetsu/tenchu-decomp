#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "humanoid.h"
#include "game_globals.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1target(void);
 *     THINK_1.C:154, 110 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a0       long xx
 *     reg   $a1       long zz
 *     reg   $a0       long vx
 *     reg   $a1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern short SR;
 *     extern struct Humanoid *StagePlayer;
 *     extern short Attrib;
 *     extern unsigned char gNannido;
 *     extern long EmergencyNotice;
 * END PSX.SYM */

/*
 * Guard idle/patrol selector when nothing is targeted: a slow activity
 * clock (actcnt) alternates look/step commands, re-rolling the patrol
 * direction after ten beats; with a target (or the alarm up) it hands
 * control to the pursue turn instead.
 */
extern Humanoid *Me_THINK_C;
extern long EmergencyNotice;

extern int rand(void);
extern s16 turn_towards_player_(s32 x_diff, s32 z_diff);

s16 Think1target(void)
{
    s32 xx;
    s32 zz;
    s32 vx;
    s32 vz;
    s32 deg;
    s16 pad;
    s32 distance;

    if (Me_THINK_C->target.model == NULL)
    {

        pad = 0;
        if ((Me_THINK_C->actcnt & (THINK_IDLE_PERIOD - 1)) == 0)
        {
            pad = PADLleft;
            if (Me_THINK_C->actflg != 0)
            {
                pad = PADLright;
            }
            if (Me_THINK_C->actscnt++ > 10)
            {
                Me_THINK_C->actflg = rand() & 1;
                Me_THINK_C->actscnt = 0;
                Me_THINK_C->actcnt++;
            }
        }
        else
        {
            Me_THINK_C->actcnt++;
        }
        return pad;
    }

    SR = SR_UNSEEN;
    if ((GameClock & 0x1f) == 0)
    {
        s32 dy;
        s32 abs_dy;
        s32 direction;

        /* Keep the coordinate pairs in one allocator identity across both tests. */
        vx = xx = StagePlayer->locate->vx - Me_THINK_C->locate->vx;
        vz = zz = StagePlayer->locate->vz - Me_THINK_C->locate->vz;
        dy = StagePlayer->locate->vy - Me_THINK_C->locate->vy;
        distance = SquareRoot0(vx * xx + vz * zz);
        deg = GetDirection(xx, zz, Me_THINK_C->rotate->vy);
        if (distance <= 4000)
        {
            /* This zero-code CFG fence gives distance the retail allocation priority. */
            if (distance != 0)
            {
                abs_dy = (dy >= 0) ? dy : -dy;
            }
            else
            {
                abs_dy = (dy >= 0) ? dy : -dy;
            }
            if (abs_dy <= 3000)
            {
                direction = (deg >= 0) ? deg : -deg;
                if (direction < 900 && StagePlayer->itmctl != ITEM_HENSHIN)
                {
                    s32 alert_time;

                    Me_THINK_C->target.archive = StagePlayer->model;
                    Attrib = (Attrib & (u16)~ATTR_PHASE) | PHASE_ALERT;
                    SetNowMotion(Me_THINK_C, MOT_STATE_DRAW, 1);
                    Me_THINK_C->chase[HUMANOID_CHASE_Z] = 0;
                    Me_THINK_C->chase[HUMANOID_CHASE_X] = 0;
                    Sound(Me_THINK_C, CHAR_VOICE_ALERT);
                    RESET_ALERT_DURATION(alert_time);
                }
            }
        }
    }

    vx = Me_THINK_C->target.model->locate.coord.t[0] - Me_THINK_C->locate->vx;
    vz = Me_THINK_C->target.model->locate.coord.t[2] - Me_THINK_C->locate->vz;
    distance = SquareRoot0(vx * vx + vz * vz);
    if (distance < 200)
    {
        return 0;
    }
    if (distance < 4000)
    {
        s32 dy;

        dy = __builtin_abs(Me_THINK_C->target.model->locate.coord.t[1] - Me_THINK_C->locate->vy);

        if (dy <= 2000)
        {
            return turn_towards_player_(vx, vz);
        }
        {

                pad = 0;
            if ((Me_THINK_C->actcnt & (THINK_IDLE_PERIOD - 1)) == 0)
            {
                pad = PADLleft;
                if (Me_THINK_C->actflg != 0)
                {
                    pad = PADLright;
                }
                if (Me_THINK_C->actscnt++ > 10)
                {
                    Me_THINK_C->actflg = rand() & 1;
                    Me_THINK_C->actscnt = 0;
                    Me_THINK_C->actcnt++;
                }
            }
            else
            {
                Me_THINK_C->actcnt++;
            }
            return pad;
        }
    }
    return turn_towards_player_(vx, vz);
}
