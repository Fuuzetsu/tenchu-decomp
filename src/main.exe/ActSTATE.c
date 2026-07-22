#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"
#include "afterimage.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSTATE(void);
 *     MOTION.C:1507, 79 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern struct Humanoid *StagePlayer;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct SVECTOR *dtV;
 *     extern short dtPAD;
 *     extern struct SVECTOR *dtR;
 *     extern struct VECTOR *dtL;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 * END PSX.SYM */

/*
 * ActSTATE (0x8002375c) — handles the humanoid state-motion family rooted at
 * 0x801: weapon draw/sheath cleanup, falls and landing reactions, and the
 * return to the normal standing motion.
 *
 * Matching notes (2,680 bytes / 670 instructions):
 *  - The two terminal motion-selection paths use signed, full-width motion-id
 *    temporaries.  Their SImode producers keep the source-level tails distinct
 *    until jump2 folds only the duplicated final motMODE store.
 *  - The redundant count test after the non-special 0x501 selection preserves
 *    the original block notes.  Both arms intentionally perform the same
 *    store; the late jump passes eliminate the test while its earlier RTL
 *    lifetime gives the target's register allocation.
 *  - The random-fall arm uses a separate block-local humanoid pointer.  This
 *    keeps the two motion-id producer islands distinct without emitted code.
 */

extern MotionManager *dtM;
extern Humanoid *Me_MOTION_C;
extern s16 motID;
extern SVECTOR *dtV;
extern s16 dtPAD;
extern SVECTOR *dtR;
extern VECTOR *dtL;
extern s16 motMODE;

extern short SetNowMotion(Humanoid *human, short mid, short move);
extern void FUN_80033bc0(VECTOR *pos, u16 spread, s16 divisor, s16 count);
extern void PadShockAR(s32 port, s32 power, s32 attack, s32 release);
extern short Sound(Humanoid *human, s16 seid);
extern int ReqLifeBar(Humanoid *h);

void ActSTATE(void)
{
    short i;

    switch ((short)(dtM->mid - 0x801))
    {
    case 0xd:
        if (dtM->count != 1)
        {
            goto draw_not_one;
        }
        {
            short cleanup_guard;
            short kind;

            kind = Me_MOTION_C->wpatk;
            switch (kind)
            {
            case 2:
                DeleteConflict(Me_MOTION_C->model->object[8]);
                DeleteConflict(Me_MOTION_C->model->object[0xb]);
                cleanup_guard = 3;
                break;
            case 3:
                DeleteConflict(Me_MOTION_C->model->object[2]);
                cleanup_guard = 3;
                break;
            case 0:
                cleanup_guard = 3;
                break;
            default:
                DeleteConflict(Me_MOTION_C->model->object[0xd]);
                DeleteConflict(Me_MOTION_C->model->object[0xe]);
                cleanup_guard = 3;
                break;
            }
            if ((cleanup_guard & 2) != 0)
            {
                if (Me_MOTION_C->illusion[0] != 0)
                {
                    DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[0]);
                    Me_MOTION_C->illusion[0] = 0;
                }
                if (Me_MOTION_C->illusion[1] != 0)
                {
                    DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[1]);
                    Me_MOTION_C->illusion[1] = 0;
                }
            }
        }
        dtM->mask = 0x7fff;
        if (Me_MOTION_C->type < 7) { if (Me_MOTION_C->type > 3) {
            if (Me_MOTION_C == StagePlayer)
            {
                SetCameraMode(CMODE_NORMAL);
            }
            {
                s32 special_motion_id;

                if ((Me_MOTION_C->attribute & 0x40) != 0)
                {
                    special_motion_id = 0x501;
                }
                else
                {
                    goto zero_motion;
                }
                motID = special_motion_id;
            }
            goto special_positive_motion;
        } }
        if ((Me_MOTION_C->attribute & 0x40) == 0)
        {
            return;
        }
        motID = 0x501;
        if (dtM->count != 0)
        {
            motMODE = 1;
        }
        else
        {
            motMODE = 1;
        }
        return;

    draw_not_one:
        if (dtM->count == dtM->motion->time / 2)
        {
            Sound(Me_MOTION_C, 0);
            EquipWeapon(Me_MOTION_C, 1);
            return;
        }
        if (dtM->count != 0)
        {
            return;
        }
        if (dtM->loop == 0)
        {
            return;
        }
        {
            Humanoid *human;
            Humanoid *player;
            long chase_z;

            human = Me_MOTION_C;
        if ((human->attribute & 3) == 0)
        {
            human->attribute |= 0x12;
            player = StagePlayer;
            human->chase[0] = player->locate->vx;
            chase_z = player->locate->vz;
            human->actscnt = 1;
            human->chase[1] = chase_z;
        }
        }
        motID = 0x501;
        motMODE = 1;
        return;

    case 0xe:
        if (dtM->count != 1)
        {
            goto sheath_not_one;
        }
        {
            short cleanup_guard;
            short kind;

            kind = Me_MOTION_C->wpatk;
            switch (kind)
            {
            case 2:
                DeleteConflict(Me_MOTION_C->model->object[8]);
                DeleteConflict(Me_MOTION_C->model->object[0xb]);
                cleanup_guard = 3;
                break;
            case 3:
                DeleteConflict(Me_MOTION_C->model->object[2]);
                cleanup_guard = 3;
                break;
            case 0:
                cleanup_guard = 3;
                break;
            default:
                DeleteConflict(Me_MOTION_C->model->object[0xd]);
                DeleteConflict(Me_MOTION_C->model->object[0xe]);
                cleanup_guard = 3;
                break;
            }
            if ((cleanup_guard & 2) != 0)
            {
                if (Me_MOTION_C->illusion[0] != 0)
                {
                    DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[0]);
                    Me_MOTION_C->illusion[0] = 0;
                }
                if (Me_MOTION_C->illusion[1] != 0)
                {
                    DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[1]);
                    Me_MOTION_C->illusion[1] = 0;
                }
            }
        }
        dtM->mask = 0x7fff;
        if ((Me_MOTION_C->attribute & 0x40) != 0)
        {
            return;
        }
        goto zero_motion;

    sheath_not_one:
        if (dtM->count == dtM->motion->time / 2)
        {
            Sound(Me_MOTION_C, 1);
            EquipWeapon(Me_MOTION_C, 0);
            return;
        }
        if (dtM->count != 0)
        {
            return;
        }
        if (dtM->loop == 0)
        {
            return;
        }
        motID = 0;
        motMODE = 1;
        return;

    case 2:
        if (dtM->count < -0x36 && dtV->vy > 200)
        {
            dtM->count = -0x1e;
        }
        if (dtV->vy > 0 && (Me_MOTION_C->pad.trig & 0x80) != 0)
        {
            motID = 0x70f;
            motMODE = 0;
        }
        {
            Humanoid *human;

            human = Me_MOTION_C;
            if ((human->attribute & 0x800) == 0 && human->map.height > 0)
            {
                goto grounded_fall;
            }
            if (dtM->count < -0x28)
            {
                if (human == StagePlayer)
                {
                    SetCameraMode(CMODE_NORMAL);
                }
                if ((Me_MOTION_C->attribute & 0x40) != 0)
                {
                    motID = 0x501;
                    motMODE = 1;
                }
                else
                {
                    motID = 0;
                    motMODE = 1;
                }
                Sound(Me_MOTION_C, 0x19);
                return;
            }
            {
                if (-0x15 < dtM->count)
                {
                    if ((human->type & 0xf0) == 0x10)
                    {
                        goto random_fall;
                    }
                    motID = 0x805;
                }
                else
                {
                    motID = 0x804;
                }
                motMODE = 0;
                return;

            random_fall:
                {
                    Humanoid *fall_human;

                    motMODE = 0;
                    motID = (rand() & 1) ? 0x1007 : 0x1008;
                    fall_human = Me_MOTION_C;
                    fall_human->life -= 10;
                    if (fall_human->life < 0)
                    {
                        fall_human->life = 0;
                    }
                    Sound(Me_MOTION_C, 8);
                    ReqLifeBar(Me_MOTION_C);
                }
                return;
            }
        }

    grounded_fall:
        if (dtM->count > -0x2e)
        {
            return;
        }
        dtV->vx >>= 1;
        dtV->vz >>= 1;
        return;

    case 3:
    case 4:
        if (dtM->count == 1 && Me_MOTION_C == StagePlayer)
        {
            PadShockAR(0, 0xff, 10, 0);
            SetCameraMode(CMODE_NORMAL);
        }
        if (dtM->count < 5 && (dtPAD & 0x20) != 0 &&
            (Me_MOTION_C->pad.trig & 0x40) != 0)
        {
            motID = 0xb09;
            motMODE = 1;
            dtR->vy += 0x800;
        }
        /* fall through */
    case 5:
        if (dtM->count == 1)
        {
            Humanoid *human;
            short sound;

            sound = 0x1a;
            human = Me_MOTION_C;
            if (motID == 0x804)
            {
                sound = 0x19;
            }
            Sound(human, sound);
            FUN_80033bc0(dtL, 300, 0xc, 10);
            if (StagePlayer == Me_MOTION_C)
            {
                if (motID == 0x805)
                {
                    PadShockAR(0, 0xff, 0, 0x1e);
                }
                else
                {
                    PadShockAR(0, 0xff, 5, 0);
                }
            }
        }
        if (dtM->count != 0)
        {
            goto damp_fall;
        }
        if (dtM->loop == 0)
        {
            goto damp_fall;
        }
        if (motID == 0x806)
        {
            CamState.CriticalHit = 1;
        }
        if (Me_MOTION_C == StagePlayer)
        {
            ((s16 (*)(s32))SetCameraMode)(0);
        }
        {
            s32 positive_motion_id;

            if ((Me_MOTION_C->attribute & 0x40) != 0)
            {
                positive_motion_id = 0x501;
            }
            else
            {
                goto zero_motion;
            }
            motID = positive_motion_id;
        }
        goto positive_motion;

    damp_fall:
        dtV->vx -= dtV->vx >> 2;
        dtV->vz -= dtV->vz >> 2;
        return;

    case 0:
        if (dtM->count != dtM->motion->time / 2)
        {
            if (dtM->count != 0)
            {
                return;
            }
            if (dtM->loop == 0)
            {
                return;
            }
        }
        motID = 0x600;
        motMODE = 1;
        i = MotionUpdateMode;
        if (i != 0)
        {
            i = 0;
            do
            {
                if (CVAhuman[i].human == Me_MOTION_C)
                {
                    goto motion_ready;
                }
                i++;
            } while (i < 5);
        }
        SetNowMotion(Me_MOTION_C, motID, motMODE);
        motMODE = -1;
    motion_ready:
        Sound(Me_MOTION_C, 0x13);
        return;

    case 0xf:
        if (dtM->count != 0)
        {
            return;
        }
        if (dtM->loop == 0)
        {
            return;
        }
        if (Me_MOTION_C == StagePlayer)
        {
            SetCameraMode(CMODE_NORMAL);
        }
        if ((Me_MOTION_C->attribute & 0x40) != 0)
        {
            motID = 0x501;
            break;
        }
    zero_motion:
        motID = 0;
        motMODE = 1;
        return;

    default:
    case 1:
        return;
    }
special_positive_motion:
    motMODE = 1;
    return;
positive_motion:
    motMODE = 1;
}
