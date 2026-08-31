#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"
#include "afterimage.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSTATE(void);
 *     MOTION.C:1507, 79 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 *     extern struct TCameraStatus CamState;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 * END PSX.SYM */

/*
 * ActSTATE (0x8002375c) — handles the humanoid state-motion family rooted at
 * 0x800: weapon draw/sheath cleanup, falls and landing reactions, and the
 * return to the normal standing motion.
 *
 * Matching notes (2,680 bytes / 670 instructions):
 *  - The two terminal motion-selection paths use signed, full-width motion-id
 *    temporaries.  Their SImode producers keep the source-level tails distinct
 *    until jump2 folds only the duplicated final motMODE store.
 *  - The redundant count test after the non-special MOT_ENGAGE_STANCE selection preserves
 *    the original block notes.  Both arms intentionally perform the same
 *    store; the late jump passes eliminate the test while its earlier RTL
 *    lifetime gives the target's register allocation.
 *  - The random-fall arm uses a separate block-local humanoid pointer.  This
 *    keeps the two motion-id producer islands distinct without emitted code.
 */

extern Humanoid *Me_MOTION_C;

extern void spawn_smoke_burst_(VECTOR *pos, u16 spread, s16 divisor, s16 count);
extern int ReqLifeBar(Humanoid *h);

void ActSTATE(void)
{
    short i;

    switch (dtM->mid)
    {
    case MOT_STATE_DRAW:
        if (dtM->count == 1)
        {
            {
                short cleanup_guard;
                short kind;

                kind = Me_MOTION_C->wpatk;
                switch (kind)
                {
                case WEP_ONININ:
                    DeleteConflict(Me_MOTION_C->model->object[8]);
                    DeleteConflict(Me_MOTION_C->model->object[0xb]);
                    cleanup_guard = 3;
                    break;
                case WEP_BEAST:
                    DeleteConflict(Me_MOTION_C->model->object[2]);
                    cleanup_guard = 3;
                    break;
                case WEP_NONE:
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
                        DisposeAfterimage(Me_MOTION_C->illusion[0]);
                        Me_MOTION_C->illusion[0] = 0;
                    }
                    if (Me_MOTION_C->illusion[1] != 0)
                    {
                        DisposeAfterimage(Me_MOTION_C->illusion[1]);
                        Me_MOTION_C->illusion[1] = 0;
                    }
                }
            }
            dtM->mask = 0x7fff;
            if (Me_MOTION_C->type < KERAI_KATANA)
            {
                if (Me_MOTION_C->type > AYAME_1)
                {
                    if (Me_MOTION_C == StagePlayer)
                    {
                        SetCameraMode(CMODE_NORMAL);
                    }
                    {
                        s32 stance_id;

                        if ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)
                        {
                            stance_id = MOT_ENGAGE_STANCE;
                        }
                        else
                        {
                            goto zero_motion;
                        }
                        motID = stance_id;
                    }
                    break;
                }
            }
            if ((Me_MOTION_C->attribute & ATTR_ALERT) == 0)
            {
                return;
            }
            motID = MOT_ENGAGE_STANCE;
            if (dtM->count != 0)
            {
                motMODE = 1;
            }
            else
            {
                motMODE = 1;
            }
            return;

        }
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
            if ((human->attribute & ATTR_PHASE) == 0)
            {
                human->attribute |= ATTR_SEARCH | PHASE_ALERT;
                player = StagePlayer;
                human->chase[0] = player->locate->vx;
                chase_z = player->locate->vz;
                human->actscnt = 1;
                human->chase[1] = chase_z;
            }
        }
        motID = MOT_ENGAGE_STANCE;
        motMODE = 1;
        return;

    case MOT_STATE_SHEATHE: /* stand down: sheathe (hitboxes and afterimage off),
                 * then back to idle unless still combat-ready */
        if (dtM->count == 1)
        {
            {
                short cleanup_guard;
                short kind;

                kind = Me_MOTION_C->wpatk;
                switch (kind)
                {
                case WEP_ONININ:
                    DeleteConflict(Me_MOTION_C->model->object[8]);
                    DeleteConflict(Me_MOTION_C->model->object[0xb]);
                    cleanup_guard = 3;
                    break;
                case WEP_BEAST:
                    DeleteConflict(Me_MOTION_C->model->object[2]);
                    cleanup_guard = 3;
                    break;
                case WEP_NONE:
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
                        DisposeAfterimage(Me_MOTION_C->illusion[0]);
                        Me_MOTION_C->illusion[0] = 0;
                    }
                    if (Me_MOTION_C->illusion[1] != 0)
                    {
                        DisposeAfterimage(Me_MOTION_C->illusion[1]);
                        Me_MOTION_C->illusion[1] = 0;
                    }
                }
            }
            dtM->mask = 0x7fff;
            if ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)
            {
                return;
            }
            goto zero_motion;

        }
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

    case MOT_STATE_FALL:
        if (dtM->count < -0x36 && dtV->vy > 200)
        {
            dtM->count = -0x1e;
        }
        if (dtV->vy > 0 && (Me_MOTION_C->pad.trig & PADRleft) != 0)
        {
            motID = MOT_ATTACK_DIVE;
            motMODE = 0;
        }
        {
            Humanoid *human;

            human = Me_MOTION_C;
            if ((human->attribute & ATTR_NOFLOOR) != 0 || human->map.height <= 0)
            {
                if (dtM->count < -0x28)
                {
                    if (human == StagePlayer)
                    {
                        SetCameraMode(CMODE_NORMAL);
                    }
                    if ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)
                    {
                        motID = MOT_ENGAGE_STANCE;
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
                if (dtM->count > -0x15)
                {
                    if ((human->type & PAGE_MASK) != PAGE_GUARD)
                    {
                        motID = MOT_STATE_LAND_HEAVY;
                        motMODE = 0;
                        return;
                    }
                }
                else
                {
                    motID = MOT_STATE_LAND;
                    motMODE = 0;
                    return;
                }

                {
                    Humanoid *fall_human;

                    motMODE = 0;
                    motID = (rand() & 1) ? MOT_DAMAGE_SLAM_BACK : MOT_DAMAGE_SLAM_FORE;
                    fall_human = Me_MOTION_C;
                    fall_human->life -= 10;
                    if (fall_human->life < 0)
                    {
                        fall_human->life = 0;
                    }
                    Sound(Me_MOTION_C, CHAR_VOICE_HURT_HEAVY);
                    ReqLifeBar(Me_MOTION_C);
                }
                return;
            }
        }

        if (dtM->count > -0x2e)
        {
            return;
        }
        dtV->vx >>= 1;
        dtV->vz >>= 1;
        return;

    case MOT_STATE_LAND:
    case MOT_STATE_LAND_HEAVY:
        if (dtM->count == 1 && Me_MOTION_C == StagePlayer)
        {
            PadShockAR(0, 0xff, 10, 0);
            SetCameraMode(CMODE_NORMAL);
        }
        if (dtM->count < 5 && (dtPAD & PADRright) != 0 &&
            (Me_MOTION_C->pad.trig & PADRdown) != 0)
        {
            motID = MOT_SQUAT_BACKFLIP;
            motMODE = 1;
            dtR->vy += 0x800;
        }
        /* fall through */
    case MOT_STATE_LAND_FLIP:
        if (dtM->count == 1)
        {
            Humanoid *human;
            short sound;

            sound = 0x1a;
            human = Me_MOTION_C;
            if (motID == MOT_STATE_LAND)
            {
                sound = 0x19;
            }
            Sound(human, sound);
            spawn_smoke_burst_(dtL, 300, 0xc, 10);
            if (StagePlayer == Me_MOTION_C)
            {
                if (motID == MOT_STATE_LAND_HEAVY)
                {
                    PadShockAR(0, 0xff, 0, 30);
                }
                else
                {
                    PadShockAR(0, 0xff, 5, 0);
                }
            }
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            if (motID == MOT_STATE_LAND_FLIP)
            {
                CamState.snap_pending = 1;
            }
            if (Me_MOTION_C == StagePlayer)
            {
                /* The value-typed cast is load-bearing: gcc 2.8.1's
                 * find_cross_jump compares CALL_INSN_FUNCTION_USAGE plus the
                 * pattern code, and every sibling SetCameraMode(0) call has
                 * identical 1-arg usage — only value-typing (call_value vs
                 * call) makes this one unmergeable. Retail emits a plain jal —
                 * an earlier note claiming jalr was wrong. See ActATTACK's
                 * DeleteConflict case for the two-partner analysis. */
                ((s16 (*)(s32))SetCameraMode)(CMODE_NORMAL);
            }
            {
                s32 stance_id;

                if ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)
                {
                    stance_id = MOT_ENGAGE_STANCE;
                }
                else
                {
                    goto zero_motion;
                }
                motID = stance_id;
            }
            goto positive_motion;
        }
        dtV->vx -= dtV->vx >> 2;
        dtV->vz -= dtV->vz >> 2;
        return;

    case MOT_STATE_CLIMB:
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
        motID = MOT_CHASE;
        motMODE = 1;
        if (MotionUpdateMode != 0)
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

    case MOT_STATE_PICKUP:
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
        if ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)
        {
            motID = MOT_ENGAGE_STANCE;
            break;
        }
    zero_motion:
        motID = 0;
        motMODE = 1;
        return;

    default:
    case 0x802:
        return;
    }
    motMODE = 1;
    return;
positive_motion:
    motMODE = 1;
}
