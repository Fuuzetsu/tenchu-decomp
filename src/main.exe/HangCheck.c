#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short HangCheck(void);
 *     MOTION.C:291, 51 src lines, frame 56 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+24     struct SVECTOR vect
 *     reg   $s0       long yy
 *     reg   $s0       long y
 *     reg   $s1       long ry
 *     reg   $s4       long oy
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern unsigned long *GlobalAreaMap;
 *     extern short MotionUpdateMode;
 *     extern short motMODE;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;

short HangCheck(void)
{
    SVECTOR vect;
    long yy;
    long y;
    long ry;
    long dy;
    long oy;
    long yc;
    short i;

    if ((Me_MOTION_C->type & PAGE_MASK) == PAGE_BEAST)
    {
        return 0;
    }
    if (Me_MOTION_C->map.height <= 0 || motID == MOT_JUMP_WALLKICK ||
        Me_MOTION_C->active_item == ACTIVE_ITEM_DISGUISE)
    {
        return 0;
    }
    yy = dtL->vy - Me_MOTION_C->height;
    GetMoveSpeed(&vect, dtR->vy, Me_MOTION_C->width >> 1, 0);
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, yy - 290,
                        dtL->vz + vect.vz, AREA_LEVEL_DEFAULT);
    dy = yy - 300;
    if (y < dtL->vy)
    {
        y = GetAreaMapLevel(GlobalAreaMap, dtL->vx - vect.vx, yy - 290,
                            dtL->vz - vect.vz, AREA_LEVEL_DEFAULT);
        if (y == (u32)LEVEL_NONE)
        {
            return 0;
        }
        if (dtL->vy < y)
        {
            dtL->vx -= (vect.vx >> 1);
            dtL->vz -= (vect.vz >> 1);
        }
        return 0;
    }
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx, dy, dtL->vz,
                        AREA_LEVEL_DEFAULT);
    if (y < dtL->vy - Me_MOTION_C->height)
    {
        return 0;
    }
    GetMoveSpeed(&vect, dtR->vy, (Me_MOTION_C->width >> 1) + 300, 0);
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx,
                        yy - LEDGE_PROBE_RISE, dtL->vz + vect.vz,
                        AREA_LEVEL_RETURN_DELTA);
    if (y == (u32)LEVEL_NONE || y > LEDGE_PROBE_RISE)
    {
        return 0;
    }
    dtL->vy -= (105 - y);
    if (Me_MOTION_C->status == STAT_HANG)
    {
        return 1;
    }
    ry = dtR->vy;
    oy = y;
    if (ry & 0xFF)
    {
        yc = ry & ANGLE_QUADRANT_MASK;
        if (ry & ANGLE_HALF_QUADRANT)
        {
            yc += ANGLE_QUADRANT;
        }
        ry = yc;
    }
    GetMoveSpeed(&vect, (s16)ry, (Me_MOTION_C->width >> 1) + 300, 0);
    dy = (dtL->vy - Me_MOTION_C->height) - LEDGE_PROBE_RISE;
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, dy,
                        dtL->vz + vect.vz, AREA_LEVEL_RETURN_DELTA);
    if (y == (u32)LEVEL_NONE || y > LEDGE_PROBE_RISE)
    {
        dtL->vy -= (oy - 5);
        return 0;
    }
    dtR->vy = ry;
    GetMoveSpeed(&vect, (s16)ry, (Me_MOTION_C->width >> 1) + 100, 0);
    y = GetAreaMapLevel(GlobalAreaMap, dtL->vx + vect.vx, dy,
                        dtL->vz + vect.vz, AREA_LEVEL_RETURN_DELTA);
    if (y != (u32)LEVEL_NONE && y <= LEDGE_PROBE_RISE)
    {
        GetMoveSpeed(&vect, dtR->vy, -200, 0);
        dtL->vx += vect.vx;
        dtL->vz += vect.vz;
        dtL->vy -= (105 - y);
    }
    motID = MOT_HANG_CATCH;
    motMODE = MOTION_MOVE_APPLY;
    SET_NOW_MOTION_UNLESS_CVA(goto found);
found:
    Sound(Me_MOTION_C, SE_LEDGE_GRIP);
    if (StagePlayer != Me_MOTION_C)
    {
        return -1;
    }
    PadShockAR(PAD_PORT_1, RUMBLE_POWER_HALF, RUMBLE_ATTACK_NONE, RUMBLE_RELEASE_LONG);
    return -1;
}
