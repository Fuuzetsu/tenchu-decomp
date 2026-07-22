#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "humanoid.h"
#include "game_globals.h"
#include "item.h"
#include "padcmd.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3attack(void);
 *     THINK_3.C:70, 29 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $v0       short rng
 *     reg   $s0       short pad
 *     reg   $a2       short idx
 *
 * Globals it touches, as the original declared them:
 *     extern short SR;
 *     extern long Distance;
 *     extern short Degree;
 *     extern short EngageLevel;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

/*
 * Select the attack controls for an alerted humanoid.  The weapon class
 * determines the turn and distance thresholds; close targets are attacked,
 * distant targets are approached, and item use is considered while idle.
 *
 * The status-7 path deliberately has its own literal return.  GCC merges its
 * short-return conversion with the final return, but the extra control-flow
 * boundary keeps that conversion above the epilogue restores, as in retail.
 */

extern Humanoid *Me_THINK_C;
extern s16 atkd[4];

extern s16 SuccessionAttack(s32 dist, s16 degree);
extern s16 ItemUse(void);

s16 Think3attack(void)
{
    s16 rng;
    s16 pad;
    s16 idx;

    pad = 0;
    idx = Me_THINK_C->wpatk >> 4;

    if (Me_THINK_C->status == 7)
    {
        if (idx != 3)
        {
            pad = SuccessionAttack(3000, 1500);
        }
        else
        {
            pad = SuccessionAttack(20000, 500);
        }
        return pad;
    }

    if (SR != -2 &&
        ((idx == 3 && Distance < 14000) || Distance < 10000))
    {
        SR = 0;
    }

    if ((s16)((4 - idx) * Me_THINK_C->turn) < Degree)
    {
        pad = 0x2000;
    }
    else if (Degree < -(s16)((4 - idx) * Me_THINK_C->turn))
    {
        pad = -0x8000;
    }

    if (idx != 3)
    {
        rng = atkd[idx] / 2;
    }
    else
    {
        rng = 4000;
    }

    if (Distance < rng)
    {
        if (__builtin_abs(Degree) < 1000 &&
            Me_THINK_C->motion->count == 0)
        {
            if (Distance < 2000)
            {
                if (rand() % (EngageLevel + 1) != 0)
                {
                    pad |= 0x80;
                    goto action_ready;
                }
                pad = 0xa0;
                goto action_ready;
            }
            goto add_attack;
        }
        pad |= 0x4000;
        goto action_ready;
    }

    if (Distance < atkd[idx])
    {
        if (idx == 3)
        {
            if (pad == 0 && rand() % (EngageLevel * 4) == 0)
            {
                pad = 0x80;
            }
            goto action_ready;
        }

        if (__builtin_abs(Degree) < 100)
        {
            if (atkd[idx] - 1000 < Distance)
            {
                pad = SetCommand(&Me_THINK_C->pad, 0x21);
                goto action_ready;
            }
        }

        if (Me_THINK_C->motion->count == 0)
        {
            if (__builtin_abs(Degree) < 1200)
            {
add_attack:
                pad |= 0x80;
                goto action_ready;
            }
        }

        if (rng + 500 < Distance)
        {
            pad |= 0x1000;
        }
        goto action_ready;
    }

    if (Me_THINK_C->status != 5)
    {
        goto action_ready;
    }

    if (StagePlayer->status == 14)
    {
        s32 command;
        s32 random;

        random = rand();
        command = 4;
        if ((random & 1) != 0)
        {
            command = 3;
        }
        pad = SetCommand(&Me_THINK_C->pad, command);
        goto action_ready;
    }

    if (Me_THINK_C->motion->count != 0)
    {
        goto use_item;
    }
    if (rand() % 3 != 0)
    {
        goto use_item;
    }
    pad = SetCommand(&Me_THINK_C->pad, 1);
    goto action_ready;

use_item:
    ItemUse();

action_ready:
    if (Me_THINK_C->motion->count == 0 &&
        rand() % 30 == 0 &&
        Me_THINK_C->status == 5)
    {
        SetNowMotion(Me_THINK_C, 0x713, 1);
    }

    return pad;
}

// Ghidra decompilation (reference):
//
//
// short Think3attack(void)
//
// {
//   short sVar1;
//   ushort uVar2;
//   uint uVar3;
//   int iVar4;
//   int iVar5;
//   long dist;
//   short cmd;
//   short sVar6;
//
//   uVar2 = 0;
//   iVar5 = (int)((uint)(ushort)Me_THINK_C->wpatk << 0x10) >> 0x14;
//   if (Me_THINK_C->status == 7) {
//     dist = 3000;
//     if (iVar5 == 3) {
//       dist = 20000;
//       sVar1 = 500;
//     }
//     else {
//       sVar1 = 0x5dc;
//     }
//     sVar1 = SuccessionAttack(dist,sVar1);
//     return sVar1;
//   }
//   if ((SR != -2) && (((iVar5 == 3 && (Distance < 14000)) || (Distance < 10000)))) {
//     SR = 0;
//   }
//   sVar1 = Me_THINK_C->wpatk >> 4;
//   iVar5 = (int)(short)((4 - sVar1) * Me_THINK_C->turn);
//   if (iVar5 < Degree) {
//     uVar2 = 0x2000;
//   }
//   else if ((int)Degree < -iVar5) {
//     uVar2 = 0x8000;
//   }
//   if (sVar1 == 3) {
//     sVar6 = 4000;
//   }
//   else {
//     sVar6 = (short)((uint)(((int)((uint)(ushort)atkd[sVar1] << 0x10) >> 0x10) -
//                           ((int)((uint)(ushort)atkd[sVar1] << 0x10) >> 0x1f)) >> 1);
//   }
//   if (Distance < sVar6) {
//     iVar5 = (int)Degree;
//     if (iVar5 < 0) {
//       iVar5 = -iVar5;
//     }
//     if ((999 < iVar5) || (Me_THINK_C->motion->count != 0)) {
//       uVar2 = uVar2 | 0x4000;
//       goto LAB_8002d488;
//     }
//     if (Distance < 2000) {
//       iVar5 = rand();
//       iVar4 = EngageLevel + 1;
//       if (iVar4 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar4 == -1) && (iVar5 == -0x80000000)) {
//         trap(0x1800);
//       }
//       uVar2 = uVar2 | 0x80;
//       if (iVar5 % iVar4 == 0) {
//         uVar2 = 0xa0;
//       }
//       goto LAB_8002d488;
//     }
// LAB_8002d3a4:
//     uVar2 = uVar2 | 0x80;
//     goto LAB_8002d488;
//   }
//   if (Distance < atkd[sVar1]) {
//     if (sVar1 == 3) {
//       if (uVar2 == 0) {
//         iVar5 = rand();
//         iVar4 = (int)EngageLevel << 2;
//         if (iVar4 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar4 == -1) && (iVar5 == -0x80000000)) {
//           trap(0x1800);
//         }
//         if (iVar5 % iVar4 == 0) {
//           uVar2 = 0x80;
//         }
//       }
//       goto LAB_8002d488;
//     }
//     iVar5 = (int)Degree;
//     if (iVar5 < 0) {
//       iVar5 = -iVar5;
//     }
//     if ((99 < iVar5) || (cmd = 0x21, Distance <= atkd[sVar1] + -1000)) {
//       if (Me_THINK_C->motion->count == 0) {
//         iVar5 = (int)Degree;
//         if (iVar5 < 0) {
//           iVar5 = -iVar5;
//         }
//         if (iVar5 < 0x4b0) goto LAB_8002d3a4;
//       }
//       if (sVar6 + 500 < Distance) {
//         uVar2 = uVar2 | 0x1000;
//       }
//       goto LAB_8002d488;
//     }
//   }
//   else {
//     if (Me_THINK_C->status != 5) goto LAB_8002d488;
//     if (StagePlayer->status != 0xe) {
//       if (Me_THINK_C->motion->count == 0) {
//         iVar5 = rand();
//         cmd = 1;
//         if (iVar5 == (iVar5 / 3) * 3) goto LAB_8002d470;
//       }
//       ItemUse();
//       goto LAB_8002d488;
//     }
//     uVar3 = rand();
//     cmd = 4;
//     if ((uVar3 & 1) != 0) {
//       cmd = 3;
//     }
//   }
// LAB_8002d470:
//   uVar2 = SetCommand(&Me_THINK_C->pad,cmd);
// LAB_8002d488:
//   if (((Me_THINK_C->motion->count == 0) && (iVar5 = rand(), iVar5 == (iVar5 / 0x1e) * 0x1e)) &&
//      (Me_THINK_C->status == 5)) {
//     SetNowMotion(Me_THINK_C,0x713,1);
//   }
//   return uVar2;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? ItemUse(s32, s16);                                /* extern */
// s32 SetCommand(s32, s16);                           /* extern */
// ? SetNowMotion(void *, ?, ?, s32);                  /* extern */
// s32 SuccessionAttack(?, ?);                         /* extern */
// s32 rand(s32, s32, s16, s16);                       /* extern */
// extern s16 Degree;
// extern s32 Distance;
// extern s16 EngageLevel;
// extern void *Me_THINK_C;
// extern s16 SR;
// extern void *StagePlayer;
// extern ? atkd;
//
// s32 Think3attack(void) {
//     ? var_a0;
//     ? var_a1;
//     s16 temp_lo;
//     s16 temp_v1_2;
//     s16 var_a1_2;
//     s16 var_v0_3;
//     s16 var_v0_4;
//     s16 var_v0_6;
//     s32 temp_a0;
//     s32 temp_ret;
//     s32 temp_ret_2;
//     s32 temp_v0_2;
//     s32 temp_v0_3;
//     s32 temp_v1;
//     s32 var_s0;
//     s32 var_v0;
//     s32 var_v0_2;
//     s32 var_v0_5;
//     s32 var_v1;
//     u16 temp_v0;
//     u32 var_a2;
//     void *var_a0_2;
//
//     var_s0 = 0;
//     temp_a0 = (s32) (Me_THINK_C->unk8E << 0x10) >> 0x14;
//     if (Me_THINK_C->unk2 == 7) {
//         var_a0 = 0xBB8;
//         if (temp_a0 != 3) {
//             var_a1 = 0x5DC;
//         } else {
//             var_a0 = 0x4E20;
//             var_a1 = 0x1F4;
//         }
//         var_s0 = SuccessionAttack(var_a0, var_a1);
//         goto block_59;
//     }
//     if (SR != -2) {
//         if (((temp_a0 == 3) && (Distance < 0x36B0)) || (var_v1 = temp_a0 << 0x10, ((Distance < 0x2710) != 0))) {
//             SR = 0;
//             goto block_10;
//         }
//     } else {
// block_10:
//         var_v1 = temp_a0 << 0x10;
//     }
//     temp_lo = (4 - (var_v1 >> 0x10)) * Me_THINK_C->unk6;
//     if (temp_lo < Degree) {
//         var_s0 = 0x2000;
//         goto block_15;
//     }
//     var_v0_2 = temp_a0 << 0x10;
//     if (Degree < -temp_lo) {
//         var_s0 = -0x8000;
// block_15:
//         var_v0_2 = temp_a0 << 0x10;
//     }
//     temp_v1 = var_v0_2 >> 0x10;
//     if (temp_v1 != 3) {
//         temp_v0 = *((temp_v1 * 2) + &atkd);
//         var_a2 = (u32) ((s16) temp_v0 + ((u32) (temp_v0 << 0x10) >> 0x1F)) >> 1;
//     } else {
//         var_a2 = 0xFA0;
//     }
//     if (Distance < (s16) var_a2) {
//         var_v0_3 = Degree;
//         if (var_v0_3 < 0) {
//             var_v0_3 = -var_v0_3;
//         }
//         if ((var_v0_3 < 0x3E8) && (Me_THINK_C->unk5C->unk2 == 0)) {
//             if (Distance < 0x7D0) {
//                 var_s0 |= 0x80;
//                 if ((rand(Distance, temp_a0, (s16) var_a2, temp_lo) % (s32) (EngageLevel + 1)) == 0) {
//                     var_s0 = 0xA0;
//                 }
//             } else {
//                 goto block_42;
//             }
//         } else {
//             var_s0 |= 0x4000;
//         }
//     } else {
//         var_a1_2 = (s16) temp_a0;
//         temp_v1_2 = *((var_a1_2 * 2) + &atkd);
//         if (Distance < temp_v1_2) {
//             if (var_a1_2 == 3) {
//                 if (((var_s0 << 0x10) == 0) && ((rand(Distance, (s32) var_a1_2, (s16) var_a2, temp_lo) % (s32) (EngageLevel * 4)) == 0)) {
//                     var_s0 = 0x80;
//                 }
//             } else {
//                 var_v0_4 = Degree;
//                 if (var_v0_4 < 0) {
//                     var_v0_4 = -var_v0_4;
//                 }
//                 if (var_v0_4 < 0x64) {
//                     var_a1_2 = 0x21;
//                     if ((temp_v1_2 - 0x3E8) < Distance) {
//                         goto block_53;
//                     }
//                 }
//                 var_v0_5 = var_a2 << 0x10;
//                 if (Me_THINK_C->unk5C->unk2 == 0) {
//                     var_v0_6 = Degree;
//                     if (var_v0_6 < 0) {
//                         var_v0_6 = -var_v0_6;
//                     }
//                     var_v0_5 = var_a2 << 0x10;
//                     if (var_v0_6 < 0x4B0) {
// block_42:
//                         var_s0 |= 0x80;
//                     } else {
//                         goto block_43;
//                     }
//                 } else {
// block_43:
//                     if (((var_v0_5 >> 0x10) + 0x1F4) < Distance) {
//                         var_s0 |= 0x1000;
//                     }
//                 }
//             }
//         } else {
//             var_a0_2 = Me_THINK_C;
//             if (var_a0_2->unk2 == 5) {
//                 if (StagePlayer->unk2 == 0xE) {
//                     var_a1_2 = 4;
//                     if (rand((s32) var_a0_2, (s32) var_a1_2, (s16) var_a2, temp_lo) & 1) {
//                         var_a1_2 = 3;
//                     }
//                     goto block_53;
//                 }
//                 if ((var_a0_2->unk5C->unk2 == 0) && (temp_ret = rand((s32) var_a0_2, (s32) var_a1_2, (s16) var_a2, temp_lo), temp_v0_2 = temp_ret, var_a0_2 = (void *) (temp_ret / 3), var_a1_2 = 1, (temp_v0_2 == (((s32) var_a0_2 * 2) + var_a0_2)))) {
// block_53:
//                     var_s0 = SetCommand(Me_THINK_C + 0x10, var_a1_2);
//                 } else {
//                     ItemUse((s32) var_a0_2, var_a1_2);
//                 }
//             }
//         }
//     }
//     var_v0 = var_s0 << 0x10;
//     if (Me_THINK_C->unk5C->unk2 == 0) {
//         temp_ret_2 = rand();
//         temp_v0_3 = temp_ret_2;
//         var_v0 = var_s0 << 0x10;
//         if (temp_v0_3 == ((temp_ret_2 / 30) * 0x1E)) {
//             var_v0 = var_s0 << 0x10;
//             if (Me_THINK_C->unk2 == 5) {
//                 SetNowMotion(Me_THINK_C, 0x713, 1, MULT_HI(temp_v0_3, 0x88888889));
// block_59:
//                 var_v0 = var_s0 << 0x10;
//             }
//         }
//     }
//     return var_v0 >> 0x10;
// }
