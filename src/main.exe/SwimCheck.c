#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SwimCheck(void);
 *     MOTION.C:219, 33 src lines, frame 40 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct VECTOR vect
 *     reg   $a2       struct ModelArchiveType * mdl
 *     reg   $a1       short i
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern struct MotionManager *dtM;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct VECTOR *dtL;
 *     extern struct Humanoid *StagePlayer;
 *     extern short ActionHalt;
 *     extern short motMODE;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;

extern void set_model_hide_(Humanoid *human, short hide);
extern int ReqLifeBar(Humanoid *h);
extern void reset_alert_duration(void);

short SwimCheck(void)
{
    character_status status;
    short i;
    VECTOR vect;
    VECTOR *locate;
    int object_id;
    int r;
    u16 motion;
    long width;

    if (Me_MOTION_C->map.height <= 0)
    {
        if ((Me_MOTION_C->map.attrib & MAP_WATER) == 0)
        {
            return 0;
        }
        status = Me_MOTION_C->status;
        if (status == STAT_KAGI)
        {
            return 0;
        }
        if (status == STAT_SWIM)
        {
            goto return_one;
        }
        if (status == STAT_DEAD)
        {
            if (motID == MOT_DEAD_DROWN)
            {
                goto return_one;
            }
            if (dtM->loop == MOTION_LOOP_DISABLED)
            {
                return 0;
            }
        }

        if (Me_MOTION_C->status != STAT_SQUAT)
        {
            object_id = (*Me_MOTION_C->model->object)->id;
            if (object_id >= 0)
            {
                locate = dtL;
                dtL->vx = ConflictObject[object_id].position.vx;
                locate->vz = ConflictObject[object_id].position.vz;
            }
        }

        vect.vy = Me_MOTION_C->map.level;
        i = 0;
        do
        {
            r = rand();
            width = Me_MOTION_C->width;
            vect.vx = dtL->vx + (r % width) * 2 - width;
            r = rand();
            width = Me_MOTION_C->width;
            vect.vz = dtL->vz + (r % width) * 2 - width;
            SetSplash(&vect, (rand() & 7) << FIXED_SHIFT,
                      (rand() & 7) << FIXED_SHIFT, 6);
            i++;
        } while (i < 20);

        AttackCancelControl(ATTACK_CANCEL_ALL);
        if (Me_MOTION_C == StagePlayer)
        {
            SetCameraMode(CMODE_SWIM);
            PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX, RUMBLE_ATTACK_NORMAL, RUMBLE_RELEASE_NONE);
        }
        ActionHalt = ACTION_HALT_NONE;
        set_model_hide_(Me_MOTION_C, 1);
        motion = GetMotionID(dtM, MOT_SWIM);
        if ((s16)motion < 0 || Me_MOTION_C->life == 0)
        {
            SET_MOTION(MOT_DEAD_DROWN, MOTION_MOVE_APPLY);
            Sound(Me_MOTION_C, CHAR_VOICE_HURT_HEAVY);
            Me_MOTION_C->life = 0;
            ReqLifeBar(Me_MOTION_C);
        }
        else
        {
            SET_MOTION(MOT_SWIM, MOTION_MOVE_APPLY);
        }

        SET_NOW_MOTION_UNLESS_CVA(goto motion_done);
    motion_done:
        Sound(Me_MOTION_C, SE_WATER_SPLASH);
        reset_alert_duration();
        goto return_one;
    }
    return 0;
return_one:
    return 1;
}
