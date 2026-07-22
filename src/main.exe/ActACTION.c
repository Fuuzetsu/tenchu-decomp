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
 *     reg   $v1       short i
 * END PSX.SYM */

/*
 * ActACTION (0x8001fb98) — controls action-motion cleanup and completion,
 * including weapon/afterimage teardown, replay transitions, sounds, and the
 * return to the normal or crouching motion.
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

extern MotionManager *dtM;
extern Humanoid *Me_MOTION_C;
extern SVECTOR *dtV;
extern s16 dtPAD;
extern s16 MotionUpdateMode;
extern s16 motID;
extern s16 motMODE;
extern HumanAnimType CVAhuman[5];

extern s16 PlayMotion(MotionManager *motion, s16 mode);
extern s16 SetNowMotion(Humanoid *human, s16 mid, s16 move);
extern void SetCameraMode(s32 mode);
extern s16 Sound(Humanoid *human, s16 id);

void ActACTION(void)
{
    short i;

    switch ((short)(dtM->mid - 0x100))
    {
    case 1:
        if (dtM->loop == 0)
            return;
        if (dtPAD == 0)
            return;
        if (Me_MOTION_C == StagePlayer)
            SetCameraMode(0);
        if (*(u16 *)&Me_MOTION_C->attribute & 0x40)
        {
            motID = 0x501;
            motMODE = 1;
            return;
        }
        goto set_normal_motion;

    case 4:
    case 5:
        if (Me_MOTION_C->life != (s16)Me_MOTION_C->lifemax ||
            (*(u16 *)&Me_MOTION_C->attribute & 1))
        {
            if (Me_MOTION_C == StagePlayer)
                SetCameraMode(0);
            if (*(u16 *)&Me_MOTION_C->attribute & 0x40)
            {
                motID = 0x501;
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
                SetCameraMode(0);
            if (*(u16 *)&Me_MOTION_C->attribute & 0x40)
            {
                motID = 0x501;
                motMODE = 1;
            }
            else
            {
                motID = 0;
                motMODE = 1;
            }
        }
        break;

    case 0:
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
            case 2:
                DeleteConflict(Me_MOTION_C->model->object[8]);
                model = Me_MOTION_C->model->object[0xb];
                break;
            case 3:
                model = Me_MOTION_C->model->object[2];
                break;
            case 0:
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
            if ((cleanup_guard & 2) == 0)
                goto afterimage_cleanup_done;
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
        afterimage_cleanup_done:
            do
            {
                motion = dtM;
                human = Me_MOTION_C;
                motion->mask = 0x7fff;
            } while (0);
            weapon = human->weapon;
            if (human->wpatk == 0x2a && weapon[3] != 0)
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
            motID = 0x100c;
            motMODE = 1;
            i = MotionUpdateMode;
            if (i != 0)
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

    case 6:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 6);
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            motID = 0x80e;
            motMODE = 1;
        }
        break;

    case 2:
    case 3:
    default:
        if (dtM->count != 0)
            return;
        if (dtM->loop == 0)
            return;
        if (Me_MOTION_C == StagePlayer)
            SetCameraMode(0);
        if ((*(u16 *)&Me_MOTION_C->attribute & 0x40) == 0)
            goto set_normal_motion;
        motID = 0x501;
        motMODE = 1;
        return;
    set_normal_motion:
        motID = 0;
        motMODE = 1;
        return;
    }
}
