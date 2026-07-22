#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSQUAT(void);
 *     MOTION.C:1712, 83 src lines, frame 40 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     reg   $s0       short turn
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern struct SVECTOR *dtR;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct SVECTOR *dtV;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct VECTOR *dtL;
 *     extern short dtCMD;
 *     extern short SelectedItem;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

/*
 * ActSQUAT (0x80024a04) — updates crouched movement, terrain blocking, and
 * crouch-state attack/item/action dispatch.
 *
 * STATUS: MATCHED — exact 1820 bytes / 455 instructions.
 *
 * Matching notes:
 *  - The demo ActSQUAT is a strong source oracle: it has the same 0x28 frame,
 *    motion cases 0-4, movement/terrain tails, command tree, and stand/stick
 *    dispatch. Retail extends that source shape with case 9, jump input, and
 *    selected-item dispatch.
 *  - `turn` is only the signed half-turn used by cases 2-4. Motion constants
 *    are assigned directly to `motID`; jump2 then sinks those stores into the
 *    target's shared `$v0` tails. Reusing `turn` for them incorrectly keeps
 *    the constants in `$s0`.
 *  - Cases 2 and 3 jump into the stationary-motion continuation inside case
 *    4. That multi-predecessor label preserves the target's physical order:
 *    continuation, case 9, then the default case.
 *  - Case 9 uses `time >> 1`, not `/ 2`; the latter adds the signed division
 *    correction sequence. The signed/unsigned dtPAD views likewise preserve
 *    the target's site-specific `lh`/`lhu` loads.
 */

extern MotionManager *dtM;
extern Humanoid *Me_MOTION_C;
extern u16 dtPAD;
extern VECTOR *dtL;
extern SVECTOR *dtR;
extern SVECTOR *dtV;
extern s16 dtCMD;
extern s16 motID;
extern s16 motMODE;
extern s16 SelectedItem;
extern u32 *GlobalAreaMap;
extern Humanoid *StagePlayer;
extern TCameraStatus CamState;
extern s32 D_80097F1C;

extern void MoveHumanoid(Humanoid *human, s16 order, s16 side);
extern long GetAreaMapLevel(unsigned long *area, long x, long y, long z,
                            int mode);
extern s16 Sound(Humanoid *human, s16 id);
extern void AttackControl(void);
extern void SetCameraMode(s32 mode);
extern void StickonCheck(void);

void ActSQUAT(void)
{
    short turn;

    turn = Me_MOTION_C->turn / 2;
    switch ((short)(dtM->mid - 0xB00))
    {
    case 0:
        if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = -1;
        }
        if (dtPAD & 0x1000)
        {
            motID = 0xB01;
            motMODE = 1;
            goto common_action;
        }
        if (dtPAD & 0x4000)
        {
            motID = 0xB02;
            motMODE = 1;
            goto common_action;
        }
        if (dtPAD & 0x2000)
        {
            motID = 0xB03;
            motMODE = 1;
            goto common_action;
        }
        if (dtPAD & 0x8000)
        {
            motID = 0xB04;
            motMODE = 1;
            goto common_action;
        }
        if (Me_MOTION_C->pad.trig & 0x40)
        {
            motID = 0xB09;
            motMODE = 1;
            dtR->vy += 0x800;
        }
        goto common_action;

    case 1:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x11);
        }
        if ((dtPAD & 0x1000) == 0)
        {
            motID = 0xB00;
            motMODE = 1;
        }
        else if (Me_MOTION_C->pad.trig & 0x40)
        {
            motID = 0xB09;
            motMODE = 1;
            dtR->vy += 0x800;
        }
        goto common_action;

    case 2:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x11);
        }
        if ((dtPAD & 0x4000) == 0)
        {
            motID = 0xB00;
            motMODE = 1;
            goto common_action;
        }
        if (dtPAD & 0xA000)
        {
            int current;
            int result;
            SVECTOR *rotation;

            rotation = dtR;
            current = rotation->vy;
            if (dtPAD & 0x2000)
            {
                result = current + turn;
            }
            else
            {
                result = current - turn;
            }
            rotation->vy = result;
            dtV->vz = 0;
            dtV->vx = 0;
            goto common_action;
        }
        goto move_if_stationary;

    case 3:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x11);
        }
        if ((dtPAD & 0x2000) == 0)
        {
            motID = 0xB00;
            motMODE = 1;
            goto common_action;
        }
        if (dtPAD & 0x4000)
        {
            dtR->vy += turn;
            dtV->vz = 0;
            dtV->vx = 0;
            goto common_action;
        }
        goto move_if_stationary;

    case 4:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x11);
        }
        if (((s16)dtPAD & 0x8000U) == 0)
        {
            motID = 0xB00;
            motMODE = 1;
            goto common_action;
        }
        if (dtPAD & 0x4000)
        {
            dtR->vy -= turn;
            dtV->vz = 0;
            dtV->vx = 0;
            goto common_action;
        }
move_if_stationary:
        if (dtV->vx == 0 && dtV->vz == 0)
        {
            MoveHumanoid(Me_MOTION_C,
                         Me_MOTION_C->motion->motion->orderspd,
                         Me_MOTION_C->motion->motion->sidespd);
        }
        goto common_action;

    case 9:
        if (dtM->count == (dtM->motion->time >> 1))
        {
            Sound(Me_MOTION_C, 0x13);
            CamState.CriticalHit = 1;
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            motID = 0xB00;
            motMODE = 1;
        }
        goto common_action;

    default:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x13);
            return;
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            motID = 0xB00;
            motMODE = 1;
            return;
        }
        if (dtV->vx == 0 && dtV->vz == 0)
        {
            return;
        }
        if (GetAreaMapLevel(GlobalAreaMap,
                            dtL->vx + dtV->vx * 4,
                            dtL->vy,
                            dtL->vz + dtV->vz * 4,
                            3) < -450)
        {
            dtL->vx -= dtV->vx;
            dtL->vz -= dtV->vz;
            dtV->vz = 0;
            dtV->vx = 0;
        }
        return;
    }

common_action:
    if (motID == 0xB09)
    {
        return;
    }
    if (motID != 0xB00 && (dtV->vx != 0 || dtV->vz != 0))
    {
        if (__builtin_abs(GetAreaMapLevel(GlobalAreaMap,
                                          dtL->vx + dtV->vx * 16,
                                          dtL->vy,
                                          dtL->vz + dtV->vz * 16,
                                          3)) >= 500)
        {
            dtV->vz = 0;
            dtV->vx = 0;
        }
    }

    if (dtCMD != 0)
    {
        if (dtCMD == 0x12)
        {
            goto command_12;
        }
        if (dtCMD < 0x13)
        {
            if (dtCMD == 0x11)
            {
                goto command_11;
            }
            return;
        }
        if (dtCMD == 0x13)
        {
            goto command_13;
        }
        if (dtCMD == 0x14)
        {
            goto command_14;
        }
        return;

command_12:
        motID = 0xB06;
        goto command_flag;

command_13:
        motID = 0xB08;
        goto command_flag;

command_11:
        motID = 0xB05;
        goto command_flag;

command_14:
        motID = 0xB07;

command_flag:
        motMODE = 1;
        return;
    }

    if (Me_MOTION_C->pad.trig & 0x80)
    {
        AttackControl();
        return;
    }
    if (Me_MOTION_C->pad.trig & 0x10)
    {
        switch ((short)(SelectedItem + 1))
        {
        case 2:
            motID = 0xE00;
            break;
        case 1:
            motID = 0x400;
            break;
        case 3:
            motID = 0xF00;
            break;
        case 6:
            motID = 0xF02;
            break;
        case 5:
            motID = 0xF02;
            break;
        case 7:
            motID = 0xF03;
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
        SoundEx(Me_MOTION_C->locate, 0xC);
        return;

item_default:
        ReqItemDefault(Me_MOTION_C, SelectedItem);
        return;
    }

    if ((dtPAD & 0x20) == 0)
    {
        if (Me_MOTION_C == StagePlayer)
        {
            SetCameraMode(0);
        }
        if (*(u16 *)&Me_MOTION_C->attribute & 0x40)
        {
            motID = 0x501;
            motMODE = 1;
            return;
        }
        motID = 0;
        motMODE = 1;
        return;
    }
    if (D_80097F1C != 0)
    {
        StickonCheck();
    }
    return;
}
