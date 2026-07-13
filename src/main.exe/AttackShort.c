#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short AttackShort(void);
 *     THINK_3.C:266, 98 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $s0       short pad
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern struct BattleType BattleDB[78];
 *     extern long Distance;
 *     extern short Degree;
 *     extern short EngageLevel;
 *     extern short Attrib;
 *     extern long GameClock;
 *     extern long AttackActionCount;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING -- 8 of 1668 bytes differ.  The draft has the exact
 * 417-instruction length and control-flow layout; only two status-7 result
 * copies differ:
 *
 *     target: move v0,s0
 *     draft:  andi v0,s0,0xffff
 *
 * A widened result pseudo prevents GCC 2.8.1 jump2 from threading away the
 * retail-only explicit zero-return block.  Narrowing `status_pad` through u16
 * keeps that layout, but materialises zero_extendhisi2 at both remaining copy
 * sites.  Signed assignments remove the masks but let jump2 merge four target
 * instructions.  Keep the pure-C near-match rather than forcing the two moves
 * with assembly.
 */

extern Humanoid *Me_THINK_C;
extern BattleType BattleDB[78];
extern s32 Distance;
extern s16 Degree;
extern s16 EngageLevel;
extern u16 Attrib;
extern s32 GameClock;
extern s32 AttackActionCount;

extern s16 AttackAnimal(void);
extern s16 ChasetoTarget(s32 distance);
extern s16 SetCommand(PADtype *pad, s16 command);
extern s16 ItemUse(void);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/AttackShort", AttackShort);
#else /* NON_MATCHING */

short AttackShort(void)
{
    MotionManager *motion;
    s16 pad;
    s32 status7_result;

    pad = 0;
    if ((Me_THINK_C->type & 0xf0) == 0xa0)
    {
        return AttackAnimal();
    }

    if (Me_THINK_C->status == 7)
    {
        s16 status_pad;
        s32 status_degree;

        status_pad = 0;
        if (Me_THINK_C->motion->count ==
            BattleDB[Me_THINK_C->warid].contfrm)
        {
            goto status7_continue;
        }
        status7_result = 0;
        goto status7_return;

status7_continue:
        if (Distance < 2000)
        {
            status_degree = Degree;
            if (status_degree < 0)
            {
                status_degree = -status_degree;
            }
            if (status_degree < 1000)
            {
                goto choose_status7;
            }
        }
        if (rand() % (EngageLevel + 1) != 0)
        {
            status7_result = (u16)status_pad;
            goto status7_return;
        }

choose_status7:
        if (Degree >= 301)
        {
            status_pad = 0x2000;
        }
        else
        {
            status_pad |= 0x80;
            if (Degree < -300)
            {
                status_pad = -0x8000;
            }
            else
            {
                goto status7_value;
            }
        }
        status_pad |= 0x80;

status7_value:
        status7_result = (u16)status_pad;
status7_return:
        return (s16)status7_result;
    }

    if (Me_THINK_C->status == 9)
    {
        return 0;
    }

    motion = Me_THINK_C->motion;
    if (motion->mid == 0x500)
    {
        return (u16)(rand() % (EngageLevel + 1) != 0) << 14;
    }

    if (Me_THINK_C->actmode == 0)
    {
        s32 raw_degree;
        s32 degree;

        if (Distance < 2500)
        {
            raw_degree = Degree;
            degree = (raw_degree >= 0) ? raw_degree : -raw_degree;
            if (degree < 1500)
            {
                if ((motion->count & 0xf) != 0)
                {
                    return 0;
                }
                if (raw_degree >= 501)
                {
                    pad = 0x2000;
                }
                else if (raw_degree < -500)
                {
                    pad = -0x8000;
                }
                pad |= 0x80;
                goto activate_and_return;
            }
        }

        pad = ChasetoTarget(2000);
        if (pad == 0)
        {
            Me_THINK_C->actmode = 1;
        }
        if (Distance >= 4001)
        {
            degree = Degree;
            if (degree < 0)
            {
                degree = -degree;
            }
            if (degree < 100 && rand() % 5 == 0)
            {
                pad = 0x1040;
            }
        }
        if ((Attrib & 0x4000) == 0)
        {
            goto return_pad;
        }

activate_and_return:
        Me_THINK_C->actmode = 1;
        goto return_pad;
    }

    if ((motion->count & 0xf) != 0)
    {
        s32 raw_degree;
        s32 degree;

        pad = Me_THINK_C->pad.data;
        if (Distance >= 1500)
        {
            goto return_pad;
        }
        raw_degree = Degree;
        degree = (raw_degree >= 0) ? raw_degree : -raw_degree;
        if (degree < 1000)
        {
            pad = 0x4000;
            goto return_pad;
        }
        if (degree >= 1501)
        {
            if (Distance < 1000)
            {
                pad = SetCommand(&Me_THINK_C->pad, 1);
                goto return_pad;
            }
            pad = 0x1000;
            goto return_pad;
        }
        if (rand() % 30 != 0)
        {
            goto return_pad;
        }
        pad = SetCommand(&Me_THINK_C->pad, 0x21);
        goto return_pad;
    }

    if (Distance >= 4001)
    {
        Humanoid *me;

        Me_THINK_C->actmode = 0;
        me = Me_THINK_C;
        Me_THINK_C->chase[1] = 0;
        me->chase[0] = 0;
        ItemUse();
        if (Distance >= 5001)
        {
            pad = 0x1040;
        }
        goto return_pad;
    }

    if ((Attrib & 0x400) != 0)
    {
        Me_THINK_C->actmode = 0;
    }

    if (Degree >= 501)
    {
        pad = 0x2000;
    }
    else if (Degree < -500)
    {
        pad = -0x8000;
    }

    if ((u32)(Distance - 1501) < 2499)
    {
        s32 attack_degree;

        attack_degree = Degree;
        if (attack_degree < 0)
        {
            attack_degree = -attack_degree;
        }
        if (attack_degree < 1000 &&
            rand() % (EngageLevel + 1) == 0 &&
            GameClock > AttackActionCount)
        {
            AttackActionCount = GameClock + EngageLevel * 10;
            if (rand() % 3 == 0)
            {
                pad = 0x4000;
            }
            return pad | 0x80;
        }
    }

    {
        s32 raw_degree;
        s32 degree;

        raw_degree = Degree;
        degree = (raw_degree >= 0) ? raw_degree : -raw_degree;
        if (degree >= 1501)
        {
            pad |= 0x4000;
            goto return_pad;
        }

        if (Distance >= 3001)
        {
            if (degree >= 200 || Distance < 3501)
            {
                goto return_with_1000;
            }
            if ((rand() & 1) != 0)
            {
                pad = SetCommand(&Me_THINK_C->pad, 1);
                goto return_pad;
            }
            if ((rand() & 1) != 0)
            {
                pad = SetCommand(&Me_THINK_C->pad, 0x21);
                goto return_pad;
            }
            ItemUse();
            goto return_pad;

return_with_1000:
            pad |= 0x1000;
            goto return_pad;
        }

        if (Distance < 1500)
        {
            if (raw_degree >= 301)
            {
                pad = SetCommand(&Me_THINK_C->pad, 3);
                goto return_pad;
            }
            if (raw_degree < -300)
            {
                pad = SetCommand(&Me_THINK_C->pad, 4);
                goto return_pad;
            }
            if (Distance >= 1000)
            {
                pad |= 0x80;
                goto return_pad;
            }
            pad = 0xa0;
            if ((rand() & 1) != 0)
            {
                pad = 0x4040;
            }
            goto return_pad;
        }

        if ((rand() & 1) == 0)
        {
            goto return_pad;
        }
        if (Degree >= 101)
        {
            pad = SetCommand(&Me_THINK_C->pad, 4);
            goto return_pad;
        }
        if (Degree < -100)
        {
            pad = SetCommand(&Me_THINK_C->pad, 3);
            goto return_pad;
        }
        pad = SetCommand(&Me_THINK_C->pad, 2);
    }

return_pad:
    return pad;
}

#endif /* NON_MATCHING */

// triage: HARD — 417 insns, mul/div, 5 callees, ~0.04 to AttackAnimal
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short AttackShort(void)
//
// {
//   Humanoid *pHVar1;
//   short sVar2;
//   ushort uVar3;
//   int iVar4;
//   uint uVar5;
//   int iVar6;
//   PADtype *pad;
//   MotionManager *pMVar7;
//
//   uVar3 = 0;
//   if ((Me_THINK_C->type & 0xf0U) == 0xa0) {
//     sVar2 = AttackAnimal();
//     return sVar2;
//   }
//   if (Me_THINK_C->status != 7) {
//     if (Me_THINK_C->status == 9) {
//       return 0;
//     }
//     pMVar7 = Me_THINK_C->motion;
//     if (pMVar7->mid == 0x500) {
//       iVar4 = rand();
//       iVar6 = EngageLevel + 1;
//       if (iVar6 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar6 == -1) && (iVar4 == -0x80000000)) {
//         trap(0x1800);
//       }
//       return (ushort)(iVar4 % iVar6 != 0) << 0xe;
//     }
//     if (Me_THINK_C->actmode != '\0') {
//       if ((pMVar7->count & 0xfU) == 0) {
//         if (4000 < Distance) {
//           Me_THINK_C->actmode = '\0';
//           pHVar1 = Me_THINK_C;
//           Me_THINK_C->chase[1] = 0;
//           pHVar1->chase[0] = 0;
//           ItemUse();
//           if (5000 < Distance) {
//             return 0x1040;
//           }
//           return 0;
//         }
//         if ((Attrib & 0x400U) != 0) {
//           Me_THINK_C->actmode = '\0';
//         }
//         if (Degree < 0x1f5) {
//           if (Degree < -500) {
//             uVar3 = 0x8000;
//           }
//         }
//         else {
//           uVar3 = 0x2000;
//         }
//         if (Distance - 0x5ddU < 0x9c3) {
//           iVar4 = (int)Degree;
//           if (iVar4 < 0) {
//             iVar4 = -iVar4;
//           }
//           if (iVar4 < 1000) {
//             iVar4 = rand();
//             iVar6 = EngageLevel + 1;
//             if (iVar6 == 0) {
//               trap(0x1c00);
//             }
//             if ((iVar6 == -1) && (iVar4 == -0x80000000)) {
//               trap(0x1800);
//             }
//             if ((iVar4 % iVar6 == 0) && (AttackActionCount < GameClock)) {
//               AttackActionCount = GameClock + EngageLevel * 10;
//               iVar4 = rand();
//               if (iVar4 == (iVar4 / 3) * 3) {
//                 uVar3 = 0x4000;
//               }
//               return uVar3 | 0x80;
//             }
//           }
//         }
//         iVar6 = (int)Degree;
//         iVar4 = iVar6;
//         if (iVar6 < 0) {
//           iVar4 = -iVar6;
//         }
//         if (0x5dc < iVar4) {
//           return uVar3 | 0x4000;
//         }
//         if (3000 < Distance) {
//           if ((199 < iVar4) || (Distance < 0xdad)) {
//             return uVar3 | 0x1000;
//           }
//           uVar5 = rand();
//           sVar2 = 1;
//           if ((uVar5 & 1) == 0) {
//             uVar5 = rand();
//             sVar2 = 0x21;
//             if ((uVar5 & 1) == 0) {
//               ItemUse();
//               return uVar3;
//             }
//             pad = &Me_THINK_C->pad;
//           }
//           else {
//             pad = &Me_THINK_C->pad;
//           }
//           goto LAB_8002e378;
//         }
//         if (Distance < 0x5dc) {
//           if (iVar6 < 0x12d) {
//             sVar2 = 4;
//             if (-0x12d < iVar6) {
//               if (999 < Distance) {
//                 return uVar3 | 0x80;
//               }
//               uVar5 = rand();
//               if ((uVar5 & 1) != 0) {
//                 return 0x4040;
//               }
//               return 0xa0;
//             }
//             pad = &Me_THINK_C->pad;
//             goto LAB_8002e378;
//           }
//           sVar2 = 3;
//         }
//         else {
//           uVar5 = rand();
//           if ((uVar5 & 1) == 0) {
//             return uVar3;
//           }
//           if (Degree < 0x65) {
//             sVar2 = 3;
//             if (Degree < -100) {
//               pad = &Me_THINK_C->pad;
//               goto LAB_8002e378;
//             }
//             sVar2 = 2;
//           }
//           else {
//             sVar2 = 4;
//           }
//         }
//       }
//       else {
//         uVar3 = (Me_THINK_C->pad).data;
//         if (0x5db < Distance) {
//           return uVar3;
//         }
//         iVar4 = (int)Degree;
//         if (iVar4 < 0) {
//           iVar4 = -iVar4;
//         }
//         if (iVar4 < 1000) {
//           return 0x4000;
//         }
//         if (0x5dc < iVar4) {
//           pad = &Me_THINK_C->pad;
//           if (999 < Distance) {
//             return 0x1000;
//           }
//           sVar2 = 1;
//           goto LAB_8002e378;
//         }
//         iVar4 = rand();
//         if (iVar4 != (iVar4 / 0x1e) * 0x1e) {
//           return uVar3;
//         }
//         sVar2 = 0x21;
//       }
//       pad = &Me_THINK_C->pad;
// LAB_8002e378:
//       sVar2 = SetCommand(pad,sVar2);
//       return sVar2;
//     }
//     if (Distance < 0x9c4) {
//       iVar6 = (int)Degree;
//       iVar4 = iVar6;
//       if (iVar6 < 0) {
//         iVar4 = -iVar6;
//       }
//       if (iVar4 < 0x5dc) {
//         if ((pMVar7->count & 0xfU) != 0) {
//           return 0;
//         }
//         if (iVar6 < 0x1f5) {
//           if (iVar6 < -500) {
//             uVar3 = 0x8000;
//           }
//         }
//         else {
//           uVar3 = 0x2000;
//         }
//         uVar3 = uVar3 | 0x80;
//         goto LAB_8002dfe4;
//       }
//     }
//     uVar3 = ChasetoTarget(2000);
//     if (uVar3 == 0) {
//       Me_THINK_C->actmode = '\x01';
//     }
//     if (4000 < Distance) {
//       iVar4 = (int)Degree;
//       if (iVar4 < 0) {
//         iVar4 = -iVar4;
//       }
//       if ((iVar4 < 100) && (iVar4 = rand(), iVar4 == (iVar4 / 5) * 5)) {
//         uVar3 = 0x1040;
//       }
//     }
//     if ((Attrib & 0x4000U) == 0) {
//       return uVar3;
//     }
// LAB_8002dfe4:
//     Me_THINK_C->actmode = '\x01';
//     return uVar3;
//   }
//   if (Me_THINK_C->motion->count != BattleDB[Me_THINK_C->warid].contfrm) {
//     return 0;
//   }
//   if (Distance < 2000) {
//     iVar4 = (int)Degree;
//     if (iVar4 < 0) {
//       iVar4 = -iVar4;
//     }
//     if (iVar4 < 1000) goto LAB_8002de10;
//   }
//   iVar4 = rand();
//   iVar6 = EngageLevel + 1;
//   if (iVar6 == 0) {
//     trap(0x1c00);
//   }
//   if ((iVar6 == -1) && (iVar4 == -0x80000000)) {
//     trap(0x1800);
//   }
//   if (iVar4 % iVar6 != 0) {
//     return 0;
//   }
// LAB_8002de10:
//   if (Degree < 0x12d) {
//     if (-0x12d < Degree) {
//       return 0x80;
//     }
//     uVar3 = 0x8000;
//   }
//   else {
//     uVar3 = 0x2000;
//   }
//   return uVar3 | 0x80;
// }
