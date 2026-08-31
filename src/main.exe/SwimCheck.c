#include "common.h"
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

/*
 * Water entry test, run when the character stands below map level on a
 * water-attributed cell: spray a ring of splashes, cancel the attack,
 * switch the player camera to swim, and start the swim motion — or the
 * drown death when the model has no swim animation (or life already ran
 * out). Returns 1 while the character belongs in the water (including
 * already swimming / drowned), 0 on dry land or while aiming the kaginawa.
 */
extern Humanoid *Me_MOTION_C;

extern void AttackCancelControl(short mode);
extern void set_model_hide_(Humanoid *human, short hide);
extern int ReqLifeBar(Humanoid *h);
extern void reset_alert_duration(void);

short SwimCheck(void)
{
    short status;
    short i;
    short j;
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
            if (dtM->loop == -1)
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
            SetSplash(&vect, (rand() & 7) << 12,
                      (rand() & 7) << 12, 6);
            i++;
        } while (i < 20);

        AttackCancelControl(3);
        if (Me_MOTION_C == StagePlayer)
        {
            SetCameraMode(CMODE_SWIM);
            PadShockAR(0, 0xff, 10, 0);
        }
        ActionHalt = 0;
        set_model_hide_(Me_MOTION_C, 1);
        motion = GetMotionID(dtM, MOT_SWIM);
        if ((s16)motion < 0 || Me_MOTION_C->life == 0)
        {
            motID = MOT_DEAD_DROWN;
            motMODE = 1;
            Sound(Me_MOTION_C, CHAR_VOICE_HURT_HEAVY);
            Me_MOTION_C->life = 0;
            ReqLifeBar(Me_MOTION_C);
        }
        else
        {
            motID = MOT_SWIM;
            motMODE = 1;
        }

        if (MotionUpdateMode != 0)
        {
            j = 0;
            do
            {
                if (CVAhuman[j].human == Me_MOTION_C)
                {
                    goto motion_done;
                }
                j++;
            } while (j < 5);
        }
        SetNowMotion(Me_MOTION_C, motID, motMODE);
        motMODE = -1;
    motion_done:
        Sound(Me_MOTION_C, SE_WATER_SPLASH);
        reset_alert_duration();
        goto return_one;
    }
    return 0;
return_one:
    return 1;
}

/* Matching notes:
 * - Keep the two color rand() calls inside SetSplash's arguments.  cc1 then
 *   materializes the first shifted color across the second call, exactly as
 *   the retail scheduler does.
 * - The two loop counters are distinct locals: sharing one makes the later
 *   CVAhuman scan inherit the splash loop's callee-saved register.
 * - Assigning the dtL alias only in the successful conflict-object arm keeps
 *   its gp load at the target's first actual use.
 */
