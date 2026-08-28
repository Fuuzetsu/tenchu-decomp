#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActCHASE(void);
 *     MOTION.C:1241, 61 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     reg   $s1       short turn
 *     reg   $v1       short i
 *     reg   $s0       long y
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern short MotionUpdateMode;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern short dtCMD;
 *     extern short SelectedItem;
 * END PSX.SYM */

/*
 * ActCHASE (0x800217dc) — updates chase movement and dispatches chase-state
 * jump, attack, and selected-item actions.
 *
 * STATUS: MATCHED — exact 1416 bytes / 354 instructions.
 */

extern Humanoid *Me_MOTION_C;
extern short HangCheck(void);
extern void JumpControl(void);
extern void AttackControl(void);
extern void spawn_smoke_burst_(VECTOR *pos, u16 spread, s16 divisor, s16 count);

void ActCHASE(void)
{
    short turn;

    turn = Me_MOTION_C->turn / 2;
    switch ((short)(dtM->mid - MOT_CHASE))
    {
    case 0:
    {
        if (dtM->count == 0 || dtM->count == dtM->motion->time / 2)
        {
            short sound;

            sound = 0x12;
            if (Me_MOTION_C->map.attrib & 8)
            {
                sound = 0x14;
            }
            Sound(Me_MOTION_C, sound);
        }

        if (dtPAD & PADLup)
        {
            if (Me_MOTION_C->attribute & 0x1000)
            {
                short i;

                motID = 0x801;
                motMODE = 0;
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
                MoveHumanoid(Me_MOTION_C, 0x23, 0);
                if (dtM->mode & 1)
                {
                    dtM->mode &= 0xfffe;
                    dtM->count = 0xd;
                }
                else
                {
                    dtM->mode |= 1;
                }
                break;
            }

            if (Me_MOTION_C->attribute & 0x400)
            {
                long y;
                long height;

                y = dtL->vy;
                height = Me_MOTION_C->map.height;
                dtL->vy = y - 400;
                Me_MOTION_C->map.height = 1;
                if (HangCheck() == 0)
                {
                    dtL->vy = y;
                    Me_MOTION_C->map.height = height;
                }
                break;
            }

            if (dtPAD & (PADLleft | PADLright))
            {
                int current;
                int result;
                MotionDataType *motion;
                SVECTOR *rotation;

                rotation = dtR;
                current = rotation->vy;
                if (dtPAD & PADLright)
                {
                    result = current + turn;
                }
                else
                {
                    result = current - turn;
                }
                rotation->vy = result;
                motion = Me_MOTION_C->motion->motion;
                MoveHumanoid(Me_MOTION_C, motion->orderspd, motion->sidespd);
                break;
            }

            if ((dtPAD & PADRright) == 0)
            {
                break;
            }
            motID = MOT_SQUAT;
        }
        else
        {
            motID = 0x501;
        }
        motMODE = 1;
        break;
    }

    case 2:
    {
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x11);
        }

        if ((dtPAD & PADLdown) == 0)
        {
            motID = 0x501;
            motMODE = 1;
        }
        else if (dtCMD == 0x22)
        {
            motID = 0x712;
            motMODE = 1;
        }
        else if (dtPAD & (PADLleft | PADLright))
        {
            int current;
            int result;
            MotionDataType *motion;
            SVECTOR *rotation;

            rotation = dtR;
            current = rotation->vy;
            if (dtPAD & PADLright)
            {
                result = current + turn * 4;
            }
            else
            {
                result = current - turn * 4;
            }
            rotation->vy = result;
            motion = Me_MOTION_C->motion->motion;
            MoveHumanoid(Me_MOTION_C, motion->orderspd, motion->sidespd);
        }

        if ((dtPAD & PADRright) == 0)
        {
            break;
        }
        if (Me_MOTION_C->pad.trig & PADRleft)
        {
            motID = 0x70c;
            motMODE = 1;
            return;
        }
        motID = MOT_SQUAT;
        motMODE = 1;
        break;
    }

    case 7:
        if (Me_MOTION_C->pad.trig & PADRleft)
        {
            AttackControl();
        }
        else if (Me_MOTION_C->pad.trig & PADRdown)
        {
            JumpControl();
        }
        /* fall through */
    case 4:
    case 5:
    case 6:
    {
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x13);
        }
        if (dtM->count < 7)
        {
            spawn_smoke_burst_(dtL, 0x96, 0xc, 1);
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            motID = 0x501;
            motMODE = 1;
        }
        return;
    }

    default:
        break;
    }

    if (dtCMD == 0x31)
    {
        motID = 0x907;
        motMODE = 0;
        MoveHumanoid(Me_MOTION_C, 0x78, 0);
        return;
    }

    if (Me_MOTION_C->pad.trig & PADRdown)
    {
        JumpControl();
        return;
    }
    if (Me_MOTION_C->pad.trig & PADRup)
    {
        switch ((short)(SelectedItem + 1))
        {
        case 2:
            motID = MOT_SYURI;
            break;
        case 1:
            motID = MOT_KAGI;
            break;
        case 3:
            motID = MOT_ITEM;
            break;
        case 6:
            motID = 0xf02;
            break;
        case 5:
            motID = 0xf02;
            break;
        case 7:
            motID = 0xf03;
            break;
        case 0:
        case 11:
            goto item_sound;
        default:
            goto item_default;
        }
        motMODE = 1;
        return;

    item_sound:
        SoundEx(Me_MOTION_C->locate, 0xc);
        return;

    item_default:
        ReqItemDefault(Me_MOTION_C, SelectedItem);
        return;
    }
    if (Me_MOTION_C->pad.trig & PADRleft)
    {
        AttackControl();
    }
}

