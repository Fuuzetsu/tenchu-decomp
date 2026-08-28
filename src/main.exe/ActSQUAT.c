#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
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
 *     extern struct TCameraStatus CamState;
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
 *    correction sequence. The signed dtPAD object and MOTION_PAD_BITS view
 *    likewise preserve the target's site-specific `lh`/`lhu` loads.
 */

extern Humanoid *Me_MOTION_C;
extern s32 PlayerSSR;

extern void AttackControl(void);
extern MapVector *StickonCheck(void);

void ActSQUAT(void)
{
    short turn;

    turn = Me_MOTION_C->turn / 2;
    switch (dtM->mid)
    {
    case 0xb00:
        if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = -1;
        }
        if (MOTION_PAD_BITS & PADLup)
        {
            motID = 0xB01;
            motMODE = 1;
            break;
        }
        if (MOTION_PAD_BITS & PADLdown)
        {
            motID = 0xB02;
            motMODE = 1;
            break;
        }
        if (MOTION_PAD_BITS & PADLright)
        {
            motID = 0xB03;
            motMODE = 1;
            break;
        }
        if (MOTION_PAD_BITS & PADLleft)
        {
            motID = 0xB04;
            motMODE = 1;
            break;
        }
        if (Me_MOTION_C->pad.trig & PADRdown)
        {
            motID = 0xB09;
            motMODE = 1;
            dtR->vy += 0x800;
        }
        break;

    case 0xb01:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x11);
        }
        if ((MOTION_PAD_BITS & PADLup) == 0)
        {
            motID = MOT_SQUAT;
            motMODE = 1;
        }
        else if (Me_MOTION_C->pad.trig & PADRdown)
        {
            motID = 0xB09;
            motMODE = 1;
            dtR->vy += 0x800;
        }
        break;

    case 0xb02:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x11);
        }
        if ((MOTION_PAD_BITS & PADLdown) == 0)
        {
            motID = MOT_SQUAT;
            motMODE = 1;
            break;
        }
        if (MOTION_PAD_BITS & (PADLleft | PADLright))
        {
            int current;
            int result;
            SVECTOR *rotation;

            rotation = dtR;
            current = rotation->vy;
            if (MOTION_PAD_BITS & PADLright)
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
            break;
        }
        goto move_if_stationary;

    case 0xb03:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x11);
        }
        if ((MOTION_PAD_BITS & PADLright) == 0)
        {
            motID = MOT_SQUAT;
            motMODE = 1;
            break;
        }
        if (MOTION_PAD_BITS & PADLdown)
        {
            dtR->vy += turn;
            dtV->vz = 0;
            dtV->vx = 0;
            break;
        }
        goto move_if_stationary;

    case 0xb04:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x11);
        }
        if ((dtPAD & PADLleft) == 0)
        {
            motID = MOT_SQUAT;
            motMODE = 1;
            break;
        }
        if (MOTION_PAD_BITS & PADLdown)
        {
            dtR->vy -= turn;
            dtV->vz = 0;
            dtV->vx = 0;
            break;
        }
    move_if_stationary:
        if (dtV->vx == 0 && dtV->vz == 0)
        {
            MoveHumanoid(Me_MOTION_C,
                         Me_MOTION_C->motion->motion->orderspd,
                         Me_MOTION_C->motion->motion->sidespd);
        }
        break;

    case 0xb09:
        if (dtM->count == (dtM->motion->time >> 1))
        {
            Sound(Me_MOTION_C, 0x13);
            CamState.snap_pending = 1;
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            motID = MOT_SQUAT;
            motMODE = 1;
        }
        break;

    default:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x13);
            return;
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            motID = MOT_SQUAT;
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
    if (motID == 0xB09)
    {
        return;
    }
    if (motID != MOT_SQUAT && (dtV->vx != 0 || dtV->vz != 0))
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
        switch (dtCMD)
        {
        case 0x11:
            motID = 0xB05;
            motMODE = 1;
            break;
        case 0x12:
            motID = 0xB06;
            motMODE = 1;
            break;
        case 0x13:
            motID = 0xB08;
            motMODE = 1;
            break;
        case 0x14:
            motID = 0xB07;
            motMODE = 1;
            break;
        }
        return;
    }

    if (Me_MOTION_C->pad.trig & PADRleft)
    {
        AttackControl();
        return;
    }
    if (Me_MOTION_C->pad.trig & PADRup)
    {
        switch (SelectedItem)
        {
        case 1:
            motID = MOT_SYURI;
            break;
        case 0:
            motID = MOT_KAGI;
            break;
        case 2:
            motID = MOT_ITEM;
            break;
        case 5:
            motID = 0xF02;
            break;
        case 4:
            motID = 0xF02;
            break;
        case 6:
            motID = 0xF03;
            break;
        case -1:
        case 10:
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

    if ((MOTION_PAD_BITS & PADRright) == 0)
    {
        if (Me_MOTION_C == StagePlayer)
        {
            SetCameraMode(CMODE_NORMAL);
        }
        if (*(u16 *)&Me_MOTION_C->attribute & ATTR_ALERT)
        {
            motID = 0x501;
            motMODE = 1;
            return;
        }
        motID = 0;
        motMODE = 1;
        return;
    }
    if (PlayerSSR != 0)
    {
        StickonCheck();
    }
    return;
}
