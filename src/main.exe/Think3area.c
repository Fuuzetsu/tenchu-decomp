#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "humanoid.h"
#include "game_globals.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3area(void);
 *     THINK_3.C:128, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     reg   $s0       short pad
 *     reg   $s2       long xx
 *     reg   $s1       long zz
 *     reg   $s3       long dist
 *     reg   $s2       long vx
 *     reg   $s1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 * END PSX.SYM */

/*
 * Think3area chooses controls while an enemy approaches its assigned area
 * point.  Weapon class 3 delegates to Think3attack; other classes either run
 * the active AttackFunc callback or steer toward point[0]/point[1].
 *
 * Matching notes:
 *  - The already-acting callback arm is written before the longer steering
 *    arm.  This matches retail's physical fallthrough layout, the same shape
 *    used by Think3hitaway.
 *  - This THINK_3.C caller sees turn_towards_player_ returning s16.  Declaring
 *    it s32 adds a deferred result copy and makes the function one instruction
 *    long; the original-width prototype keeps the return in $v0 until reorg
 *    moves it into `pad` in the following Attrib branch's delay slot.
 *  - `__builtin_abs(Degree)` gives the target's two-pseudo abssi2 expansion:
 *    the raw signed Degree remains in $v0 while the absolute result occupies
 *    $v1.  Mutating one `degree` local in place leaves a seven-byte residual.
 */

extern Humanoid *Me_THINK_C;
extern s32 Distance;
extern s16 Degree;
extern u16 Attrib;
extern s32 (*AttackFunc[])(void);

extern s16 SuccessionAttack(s32 distance, s16 degree);
extern s16 Think3attack(void);
extern s16 turn_towards_player_(s32 x_diff, s32 z_diff);
extern s16 SetCommand(PADtype *pad, s16 command);

s16 Think3area(void)
{
    s16 pad;
    s32 xx;
    s32 zz;
    s32 dist;

    pad = 0;
    if (Me_THINK_C->status == 7)
    {
        return SuccessionAttack(4000, 500);
    }

    if (Distance < 10000 && SR != -2)
    {
        SR = 0;
    }

    if (Me_THINK_C->wpatk >> 4 == 3)
    {
        return Think3attack();
    }

    xx = Me_THINK_C->point[0] - Me_THINK_C->locate->vx;
    zz = Me_THINK_C->point[1] - Me_THINK_C->locate->vz;
    dist = SquareRoot0(xx * xx + zz * zz);

    if (Me_THINK_C->actflg != 0)
    {
        pad = AttackFunc[Me_THINK_C->wpatk >> 4]();
        if (Distance < 4000)
        {
            Me_THINK_C->actcnt++;
            if (Me_THINK_C->actcnt == 0 && dist > 4000)
            {
                Me_THINK_C->actflg = 0;
            }
        }
        goto return_pad;
    }

    if ((Attrib & 0x4000) != 0)
    {
        Me_THINK_C->actflg = 1;
    }

    if (dist < 2000)
    {
        s32 degree;

        if ((Me_THINK_C->motion->count & 7) != 0)
        {
            pad = Me_THINK_C->pad.data;
        }
        else if (Degree >= 501)
        {
            pad = 0x2000;
        }
        else if (Degree < -500)
        {
            pad = -0x8000;
        }
        else if (rand() % 10 == 0)
        {
            SetNowMotion(Me_THINK_C, 0x713, 1);
            Me_THINK_C->actflg = 1;
        }

        if (Distance >= 4000)
        {
            goto return_pad;
        }

        degree = __builtin_abs(Degree);

        if (degree < 100)
        {
            pad = SetCommand(&Me_THINK_C->pad, 0x21);
        }
        else if (degree < 1000)
        {
            pad |= 0x80;
        }
        Me_THINK_C->actflg = 1;
        goto return_pad;
    }

    pad = turn_towards_player_(xx, zz);
    if ((Attrib & 0x400) != 0)
    {
        Me_THINK_C->actflg = 1;
    }
    if (Me_THINK_C->motion->count != 0)
    {
        goto return_pad;
    }
    if (Distance >= 3000)
    {
        goto return_pad;
    }
    {
        s32 degree;

        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree < 1000)
        {
            pad |= 0x80;
        }
    }

return_pad:
    return pad;
}

// triage: MEDIUM — 211 insns, mul/div, indirect-call, 7 callees, ~0.06 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// int FUN_8002d638(void)
//
// {
//   ushort uVar1;
//   int iVar2;
//   long lVar3;
//   int iVar4;
//   uint uVar5;
//
//   uVar5 = 0;
//   if (Me_THINK_C->status == 7) {
//     uVar1 = SuccessionAttack(4000,500);
//     iVar2 = (uint)uVar1 << 0x10;
//   }
//   else {
//     if ((Distance < 10000) && (SR != -2)) {
//       SR = 0;
//     }
//     if ((int)((uint)(ushort)Me_THINK_C->wpatk << 0x10) >> 0x14 == 3) {
//       uVar1 = Think3attack();
//       iVar2 = (uint)uVar1 << 0x10;
//     }
//     else {
//       iVar4 = Me_THINK_C->point[0] - Me_THINK_C->locate->vx;
//       iVar2 = Me_THINK_C->point[1] - Me_THINK_C->locate->vz;
//       lVar3 = SquareRoot0(iVar4 * iVar4 + iVar2 * iVar2);
//       if (Me_THINK_C->actflg == '\0') {
//         if ((Attrib & 0x4000U) != 0) {
//           Me_THINK_C->actflg = '\x01';
//         }
//         if (lVar3 < 2000) {
//           if ((Me_THINK_C->motion->count & 7U) == 0) {
//             if (Degree < 0x1f5) {
//               if (Degree < -500) {
//                 uVar5 = 0xffff8000;
//               }
//               else {
//                 iVar2 = rand();
//                 if (iVar2 == (iVar2 / 10) * 10) {
//                   SetNowMotion(Me_THINK_C,0x713,1);
//                   Me_THINK_C->actflg = '\x01';
//                 }
//               }
//             }
//             else {
//               uVar5 = 0x2000;
//             }
//           }
//           else {
//             uVar5 = (uint)(Me_THINK_C->pad).data;
//           }
//           iVar2 = uVar5 << 0x10;
//           if (3999 < Distance) goto LAB_8002d964;
//           iVar2 = (int)Degree;
//           if (iVar2 < 0) {
//             iVar2 = -iVar2;
//           }
//           if (iVar2 < 100) {
//             uVar1 = SetCommand(&Me_THINK_C->pad,0x21);
//             uVar5 = (uint)uVar1;
//           }
//           else if (iVar2 < 1000) {
//             uVar5 = uVar5 | 0x80;
//           }
//           Me_THINK_C->actflg = '\x01';
//         }
//         else {
//           uVar5 = FUN_8002b990(iVar4,iVar2);
//           if ((Attrib & 0x400U) != 0) {
//             Me_THINK_C->actflg = '\x01';
//           }
//           iVar2 = uVar5 << 0x10;
//           if ((Me_THINK_C->motion->count != 0) || (iVar2 = uVar5 << 0x10, 2999 < Distance))
//           goto LAB_8002d964;
//           iVar4 = (int)Degree;
//           if (iVar4 < 0) {
//             iVar4 = -iVar4;
//           }
//           iVar2 = uVar5 << 0x10;
//           if (999 < iVar4) goto LAB_8002d964;
//           uVar5 = uVar5 | 0x80;
//         }
//       }
//       else {
//         uVar5 = (*(code *)AttackFunc[(int)((uint)(ushort)Me_THINK_C->wpatk << 0x10) >> 0x14])();
//         if (Distance < 4000) {
//           Me_THINK_C->actcnt = Me_THINK_C->actcnt + '\x01';
//           iVar2 = uVar5 << 0x10;
//           if ((Me_THINK_C->actcnt == '\0') && (iVar2 = uVar5 << 0x10, 4000 < lVar3)) {
//             Me_THINK_C->actflg = '\0';
//           }
//           goto LAB_8002d964;
//         }
//       }
//       iVar2 = uVar5 << 0x10;
//     }
//   }
// LAB_8002d964:
//   return iVar2 >> 0x10;
// }
