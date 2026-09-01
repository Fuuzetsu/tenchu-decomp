#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "afterimage.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActACTION(void);
 *     MOTION.C:963, 27 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $v1       short i
 * END PSX.SYM */

/*
 * ActACTION (0x8001fb98) — controls action-motion cleanup and completion,
 * including weapon/afterimage teardown, replay transitions, sounds, and the
 * return to the normal motion, or to the weapon-drawn engage stance
 * (0x501) when ATTR_ALERT is up.
 *
 * Matching notes (1,392 bytes / 348 instructions):
 *  - The one-shot loop around the dtM/Me_MOTION_C loads and mask store leaves
 *    the original loop note for sched2.  It keeps the MOTION_MASK_ALL store at the
 *    shared cleanup join instead of duplicating it into predecessor delay
 *    slots, and preserves the target's dtM-then-Me_MOTION_C load order.
 *  - The case-1 and final motion selections write their complete terminal
 *    motID/motMODE tails independently.  jump2 then shares only the flag
 *    store, retaining both target motID stores and their branch layout.
 */

extern Humanoid *Me_MOTION_C;

extern s16 PlayMotion(MotionManager *motion, s16 mode);

void ActACTION(void)
{
    short i;

    switch (dtM->mid)
    {
    case MOT_ACTION_LOOP:
        if (dtM->loop == 0)
            return;
        if (dtPAD == 0)
            return;
        if (Me_MOTION_C == StagePlayer)
            SetCameraMode(CMODE_NORMAL);
        if (Me_MOTION_C->attribute & ATTR_ALERT)
        {
            SET_MOTION(MOT_ENGAGE_STANCE, 1);
            return;
        }
        goto set_normal_motion;

    case MOT_ACTION_FIDGET_A:
    case MOT_ACTION_FIDGET_B:
        if (Me_MOTION_C->life != Me_MOTION_C->lifemax ||
            (Me_MOTION_C->attribute & PHASE_SUSPICIOUS))
        {
            if (Me_MOTION_C == StagePlayer)
                SetCameraMode(CMODE_NORMAL);
            if (Me_MOTION_C->attribute & ATTR_ALERT)
            {
                SET_MOTION(MOT_ENGAGE_STANCE, 1);
            }
            else
            {
                SET_MOTION(0, 1);
            }
        }
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, CHAR_VOICE_IDLE);
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            if (Me_MOTION_C == StagePlayer)
                SetCameraMode(CMODE_NORMAL);
            if (Me_MOTION_C->attribute & ATTR_ALERT)
            {
                SET_MOTION(MOT_ENGAGE_STANCE, 1);
            }
            else
            {
                SET_MOTION(0, 1);
            }
        }
        break;

    case MOT_ACTION:
        if (dtM->count == 1)
        {
            s16 cleanup_guard;
            MotionManager *motion;
            Humanoid *human;
            OrnamentType **weapon;

            switch (Me_MOTION_C->wpatk)
            {
            case FIST:
                DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_0]);
                DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_1]);
                break;
            case JAW:
                DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_BEAST_HAND_0]);
                break;
            case NO_WEAPON:
                cleanup_guard = 3;
                goto skip_afterimage_cleanup;
            default:
                DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0]);
                DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_1]);
                break;
            }
            cleanup_guard = 3;
        skip_afterimage_cleanup:
            if (cleanup_guard & 2)
            {
                if (Me_MOTION_C->illusion[0] != 0)
                {
                    DisposeAfterimage(Me_MOTION_C->illusion[0]);
                    Me_MOTION_C->illusion[0] = 0;
                }
                if (Me_MOTION_C->illusion[1] != 0)
                {
                    DisposeAfterimage(Me_MOTION_C->illusion[1]);
                    Me_MOTION_C->illusion[1] = 0;
                }
            }
            motion = dtM;
            human = Me_MOTION_C;
            motion->mask = MOTION_MASK_ALL;
            /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
            do
            {
            } while (0);
            weapon = human->weapon;
            if (human->wpatk == KATANAL && weapon[WEAPON_SLOT_INACTIVE_1] != 0)
            {
                weapon[WEAPON_SLOT_INACTIVE_0] = human->weapon[WEAPON_SLOT_ACTIVE_0];
                human->weapon[WEAPON_SLOT_ACTIVE_0] = weapon[WEAPON_SLOT_INACTIVE_1];
                weapon[WEAPON_SLOT_INACTIVE_1] = 0;
                Sound(human, CHAR_SE_WEAPON_CHANGE_B);
            }
        }
        {
            if (dtM->count == 0 && dtM->loop > 0)
            {
                dtM->count = dtM->motion->time - 1;
                PlayMotion(dtM, 1);
                dtM->loop = -1;
                dtV->vz = 0;
                dtV->vx = 0;
            }
        }
        if (dtM->loop == -1 && dtPAD != 0)
        {
            SET_MOTION(MOT_DAMAGE_GETUP, 1);
            if (MotionUpdateMode != 0)
            {
                i = 0;
                do
                {
                    if (CVAhuman[i].human == Me_MOTION_C)
                        goto motion_ready;
                    i++;
                } while (i < N_CVA_HUMANS);
            }
            SetNowMotion(Me_MOTION_C, motID, motMODE);
            motMODE = -1;
        motion_ready:
            dtM->count = -0xf;
        }
        break;

    case MOT_ACTION_NOTICE:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, CHAR_VOICE_HURT);
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            SET_MOTION(MOT_STATE_DRAW, 1);
        }
        break;

    case MOT_ACTION_GESTURE:
    /* A hole in the MOT_ enum, between MOT_ACTION_GESTURE 0x102 and MOT_ACTION_FIDGET_A 0x104. It is not in MOTcommon and
     * nothing in main.exe plays it, so it arrives from a character's own
     * mtbl and its meaning is not recoverable here -- hence the digit. */
    case 0x103:
    default:
        if (dtM->count != 0)
            return;
        if (dtM->loop == 0)
            return;
        if (Me_MOTION_C == StagePlayer)
            SetCameraMode(CMODE_NORMAL);
        if ((Me_MOTION_C->attribute & ATTR_ALERT) == 0)
            goto set_normal_motion;
        SET_MOTION(MOT_ENGAGE_STANCE, 1);
        return;
    set_normal_motion:
        SET_MOTION(0, 1);
        return;
    }
}
