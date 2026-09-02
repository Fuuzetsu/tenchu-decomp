#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActHANG(void);
 *     MOTION.C:1663, 45 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       long y
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR *dtV;
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern struct VECTOR *dtL;
 *     extern short motID;
 *     extern short motMODE;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;

extern short HangCheck(void);

void ActHANG(void)
{
    long y;

    dtV->vy = 0;
    switch (dtM->mid)
    {
    case MOT_HANG:
        if (dtPAD & PADLdown)
        {
            y = dtL->vy;
            do
            {
                y += 100;
                dtL->vy = y;
            } while (HangCheck() != 0);
            SET_MOTION(MOT_STATE_FALL, MOTION_MOVE_NONE);
        }
        else if (dtPAD & PADLright)
        {
            SET_MOTION(MOT_HANG_SHIMMY_RIGHT, MOTION_MOVE_APPLY);
        }
        else if (dtPAD & PADLleft)
        {
            SET_MOTION(MOT_HANG_SHIMMY_LEFT, MOTION_MOVE_APPLY);
        }
        else if ((dtPAD & PADLup) &&
                 GetAreaMapLevel(GlobalAreaMap, dtL->vx, dtL->vy - 2000,
                                 dtL->vz, AREA_LEVEL_STEP_DOWN) !=
                     (u32)LEVEL_NONE)
        {
            SET_MOTION(MOT_HANG_PULLUP, MOTION_MOVE_APPLY);
        }
        break;
    case MOT_HANG_SHIMMY_RIGHT:
    case MOT_HANG_SHIMMY_LEFT:
        if ((dtPAD & (PADLleft | PADLright)) == 0)
        {
            SET_MOTION(MOT_HANG, MOTION_MOVE_APPLY);
        }
        else if (HangCheck() == 0)
        {
            SET_MOTION(MOT_STATE_FALL, MOTION_MOVE_NONE);
        }
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_LEDGE_GRIP);
        }
        break;
    case MOT_HANG_PULLUP:
        if (dtM->count == 0 && dtM->loop != 0)
        {
            if (Me_MOTION_C == StagePlayer)
            {
                SetCameraMode(CMODE_NORMAL);
            }
            if (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN)
            {
                SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
                return;
            }
            SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
            return;
        }
        if (dtM->count >= 0)
        {
            dtV->vy = -35;
            if (dtM->count > 40)
            {
                MoveHumanoid(Me_MOTION_C, 100, 0);
            }
        }
        break;
    case MOT_HANG_CATCH:
        if (dtM->count == 0 && dtM->loop != 0)
        {
            SET_MOTION(MOT_HANG, MOTION_MOVE_APPLY);
        }
        break;
    }
    if (Me_MOTION_C->attribute & ATTR_PUSH)
    {
        /* Shoved by a conflict object while hanging: knocked off. */
        MoveHumanoid(Me_MOTION_C, -10, 0);
        SET_MOTION(MOT_STATE_FALL, MOTION_MOVE_NONE);
    }
}
