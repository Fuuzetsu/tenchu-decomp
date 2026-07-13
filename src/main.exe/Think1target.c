#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1target(void);
 *     THINK_1.C:154, 110 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * END PSX.SYM */

extern Humanoid *Me_THINK_C;
extern Humanoid *StagePlayer;
extern s32 GameClock;
extern s16 SR;
extern u16 Attrib;
extern s32 FRAMES_UNTIL_END_OF_ALERT;
extern u8 D_80010058;

extern s32 SquareRoot0(s32 value);
extern s16 GetDirection(s32 dx, s32 dz, s16 roty);
extern s16 SetNowMotion(Humanoid *human, s16 mid, s16 move);
extern s16 Sound(Humanoid *human, s16 id);
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

    if (Me_THINK_C->target == NULL)
    {
        u8 actcnt;
        u8 old_actscnt;

        actcnt = Me_THINK_C->actcnt;
        pad = 0;
        if ((actcnt & 0x7f) == 0)
        {
            pad = -0x8000;
            if (Me_THINK_C->actflg != 0)
            {
                pad = 0x2000;
            }
            old_actscnt = Me_THINK_C->actscnt;
            Me_THINK_C->actscnt = old_actscnt + 1;
            if (old_actscnt > 10)
            {
                Me_THINK_C->actflg = rand() & 1;
                Me_THINK_C->actscnt = 0;
                Me_THINK_C->actcnt = Me_THINK_C->actcnt + 1;
            }
        }
        else
        {
            Me_THINK_C->actcnt = actcnt + 1;
        }
        return pad;
    }

    SR = -1;
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
        if (distance < 4001)
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
            if (abs_dy < 3001)
            {
                direction = (deg >= 0) ? deg : -deg;
                if (direction < 900 && StagePlayer->active_item != 11)
                {
                    s32 alert_time;

                    Me_THINK_C->target = (ModelType *)StagePlayer->model;
                    Attrib = (Attrib & 0xfffc) | 2;
                    SetNowMotion(Me_THINK_C, 0x80e, 1);
                    Me_THINK_C->chase[1] = 0;
                    Me_THINK_C->chase[0] = 0;
                    Sound(Me_THINK_C, 13);
                    alert_time = 300;
                    if (D_80010058 == 2)
                    {
                        alert_time = 600;
                    }
                    FRAMES_UNTIL_END_OF_ALERT = alert_time;
                }
            }
        }
    }

    vx = Me_THINK_C->target->locate.coord.t[0] - Me_THINK_C->locate->vx;
    vz = Me_THINK_C->target->locate.coord.t[2] - Me_THINK_C->locate->vz;
    distance = SquareRoot0(vx * vx + vz * vz);
    if (distance < 200)
    {
        return 0;
    }
    if (distance < 4000)
    {
        s32 dy;

        dy = __builtin_abs(Me_THINK_C->target->locate.coord.t[1] - Me_THINK_C->locate->vy);

        if (dy < 2001)
        {
            return turn_towards_player_(vx, vz);
        }
        {
            u8 actcnt;
            u8 old_actscnt;

            actcnt = Me_THINK_C->actcnt;
            pad = 0;
            if ((actcnt & 0x7f) == 0)
            {
                pad = -0x8000;
                if (Me_THINK_C->actflg != 0)
                {
                    pad = 0x2000;
                }
                old_actscnt = Me_THINK_C->actscnt;
                Me_THINK_C->actscnt = old_actscnt + 1;
                if (old_actscnt > 10)
                {
                    Me_THINK_C->actflg = rand() & 1;
                    Me_THINK_C->actscnt = 0;
                    Me_THINK_C->actcnt = Me_THINK_C->actcnt + 1;
                }
            }
            else
            {
                Me_THINK_C->actcnt = actcnt + 1;
            }
            return pad;
        }
    }
    return turn_towards_player_(vx, vz);
}
