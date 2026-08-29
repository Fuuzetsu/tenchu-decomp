#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SwimCheck(void);
 *     MOTION.C:219, 33 src lines, frame 40 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 * SwimCheck (0x8001c930) — MOTION.C's water-entry test for the
 * character currently being updated (Me_MOTION_C). Returns 1 when that
 * character belongs in the water, 0 otherwise. Bails with 0 unless the
 * character has sunk to or below ground level (map.height <= 0) on a
 * cell whose map.attrib carries MAP_WATER. Three statuses
 * short-circuit: STAT_KAGI (grapple aiming) returns 0, STAT_SWIM
 * returns 1 as already-swimming, and STAT_DEAD returns 1 once motID is
 * the 0x1108 drowning motion but 0 while the current motion is still
 * non-looping. Otherwise it commits to the entry. Unless squatting it
 * snaps dtL's x/z onto the root object's ConflictObject position, then
 * sprays 20 SetSplash particles at map.level scattered +/- the
 * character's width in x and z with random 3-bit texture offsets and
 * speed 6. AttackCancelControl(3) drops any attack; the player
 * additionally gets SetCameraMode(CMODE_SWIM) and a PadShockAR rumble.
 * ActionHalt clears and the model is hidden. Motion choice: if
 * GetMotionID reports the model has no MOT_SWIM animation, or life
 * already reached 0, it queues the 0x1108 drowning motion (a MOT_DEAD
 * variant), plays Sound 8, forces life to 0 and refreshes the life
 * bar; otherwise it queues MOT_SWIM. The queued motion is applied
 * through SetNowMotion unless MotionUpdateMode is set and the
 * character occupies one of the five CVAhuman animation slots. Either
 * way Sound 0x16 plays and reset_alert_duration runs.
 */

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
            if (motID == 0x1108)
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
            motID = 0x1108;
            motMODE = 1;
            Sound(Me_MOTION_C, 8);
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
        Sound(Me_MOTION_C, 0x16);
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
