#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "afterimage.h"

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
 *    the original loop note for sched2.  It keeps the 0x7fff literal at the
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
        if (*(u16 *)&Me_MOTION_C->attribute & ATTR_ALERT)
        {
            motID = MOT_ENGAGE_STANCE;
            motMODE = 1;
            return;
        }
        goto set_normal_motion;

    case MOT_ACTION_FIDGET_A:
    case MOT_ACTION_FIDGET_B:
        if (Me_MOTION_C->life != Me_MOTION_C->lifemax ||
            (*(u16 *)&Me_MOTION_C->attribute & PHASE_SUSPICIOUS))
        {
            if (Me_MOTION_C == StagePlayer)
                SetCameraMode(CMODE_NORMAL);
            if (*(u16 *)&Me_MOTION_C->attribute & ATTR_ALERT)
            {
                motID = MOT_ENGAGE_STANCE;
                motMODE = 1;
            }
            else
            {
                motID = 0;
                motMODE = 1;
            }
        }
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0xf);
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            if (Me_MOTION_C == StagePlayer)
                SetCameraMode(CMODE_NORMAL);
            if (*(u16 *)&Me_MOTION_C->attribute & ATTR_ALERT)
            {
                motID = MOT_ENGAGE_STANCE;
                motMODE = 1;
            }
            else
            {
                motID = 0;
                motMODE = 1;
            }
        }
        break;

    case MOT_ACTION:
        if (dtM->count == 1)
        {
            s16 kind;
            ModelType *model;
            s16 cleanup_guard;
            MotionManager *motion;
            Humanoid *human;
            OrnamentType **weapon;

            kind = Me_MOTION_C->wpatk;
            switch (kind)
            {
            case WEP_ONININ:
                DeleteConflict(Me_MOTION_C->model->object[8]);
                model = Me_MOTION_C->model->object[0xb];
                break;
            case WEP_BEAST:
                model = Me_MOTION_C->model->object[2];
                break;
            case WEP_NONE:
                cleanup_guard = 3;
                goto skip_afterimage_cleanup;
            default:
                DeleteConflict(Me_MOTION_C->model->object[0xd]);
                model = Me_MOTION_C->model->object[0xe];
                break;
            }
            DeleteConflict(model);
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
            motion->mask = 0x7fff;
            /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
            do
            {
            } while (0);
            weapon = human->weapon;
            if (human->wpatk == WEP_TWIN_KATANA && weapon[3] != 0)
            {
                weapon[2] = human->weapon[0];
                human->weapon[0] = weapon[3];
                weapon[3] = 0;
                Sound(human, 1);
            }
        }
        {
            MotionManager *motion;

            motion = dtM;
            if (dtM->count == 0 && dtM->loop > 0)
            {
                dtM->count = dtM->motion->time - 1;
                PlayMotion(motion, 1);
                dtM->loop = -1;
                dtV->vz = 0;
                dtV->vx = 0;
            }
        }
        if (dtM->loop == -1 && dtPAD != 0)
        {
            motID = MOT_DAMAGE_GETUP;
            motMODE = 1;
            if (MotionUpdateMode != 0)
            {
                i = 0;
                do
                {
                    if (CVAhuman[i].human == Me_MOTION_C)
                        goto motion_ready;
                    i++;
                } while (i < 5);
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
            Sound(Me_MOTION_C, 6);
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            motID = MOT_STATE_DRAW;
            motMODE = 1;
        }
        break;

    case MOT_ACTION_GESTURE:
    case 0x103:
    default:
        if (dtM->count != 0)
            return;
        if (dtM->loop == 0)
            return;
        if (Me_MOTION_C == StagePlayer)
            SetCameraMode(CMODE_NORMAL);
        if ((*(u16 *)&Me_MOTION_C->attribute & ATTR_ALERT) == 0)
            goto set_normal_motion;
        motID = MOT_ENGAGE_STANCE;
        motMODE = 1;
        return;
    set_normal_motion:
        motID = 0;
        motMODE = 1;
        return;
    }
}
