#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short AttackGeneral(void);
 *     THINK_3.C:368, 78 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 * AttackGeneral (0x8002e39c, 0x5a4 bytes) -- general-purpose humanoid
 * attack chooser.  The status-7 continuation gate shares AttackIndirect's
 * one-shot loop fence; the ordinary path chooses chase, turn, item, and
 * SetCommand actions from distance, facing, and EngageLevel rolls.
 *
 * Matching notes:
 *  - GameClock must use its original scalar declaration.  The equivalent
 *    unknown-size-array declaration lets delay-slot filling hoist its `lui`
 *    across the modulo guard and makes the function one instruction short.
 *  - Spell the time guard `GameClock > AttackActionCount`: comparison
 *    operand evaluation order puts the absolute GameClock load before the
 *    gp-relative action-count load, as in the target.
 *  - The cold close-range `% 4` switch needs an explicit default return.
 *    Otherwise its fallthrough becomes another predecessor of the normal
 *    range label, so CSE cannot carry the pre-switch Distance value and
 *    reloads it.  Explicit close/normal/near labels preserve the target's
 *    cold-block placement and shared SetCommand tail without any assembly.
 */

extern Humanoid *Me_THINK_C;
extern s32 Distance;
extern s16 Degree;
extern u16 Attrib;
extern s32 AttackActionCount;

extern s16 ChasetoTarget(s32 distance);
extern s16 SetCommand(PADtype *pad, s16 command);
extern s16 ItemUse(void);

short AttackGeneral(void)
{
    s16 pad;
    s16 status7_result;

    pad = 0;
    if (Me_THINK_C->status == 7)
    {
        s32 status_degree;

        do
        {
            if (Me_THINK_C->motion->count !=
                BattleDB[Me_THINK_C->warid].contfrm)
            {
                status7_result = 0;
                goto status7_return;
            }
            if (Distance < 2000)
            {
                status_degree = Degree;
                if (status_degree < 0)
                {
                    status_degree = -status_degree;
                }
                if (status_degree < 500)
                {
                    goto choose_status7;
                }
            }
            if (rand() % (EngageLevel + 1) != 0)
            {
                status7_result = pad;
                goto status7_return;
            }
        } while (0);

choose_status7:
        if (Degree >= 301)
        {
            pad = 0x2000;
        }
        else
        {
            pad |= 0x80;
            if (Degree < -300)
            {
                pad = -0x8000;
            }
            else
            {
                goto status7_value;
            }
        }
        pad |= 0x80;

status7_value:
        status7_result = pad;
status7_return:
        return status7_result;
    }

    if (Me_THINK_C->status == 9)
    {
        return 0;
    }

    if (Me_THINK_C->motion->mid == 0x500)
    {
        return (u16)(rand() % (EngageLevel + 1) != 0) << 14;
    }

    if (Me_THINK_C->actmode == 0)
    {
        s32 chase_degree;

        pad = ChasetoTarget(3000);
        if (pad == 0 || (Attrib & 0x4000) != 0)
        {
            Me_THINK_C->actmode = 1;
        }
        if (Distance < 5001)
        {
            goto return_pad;
        }
        chase_degree = Degree;
        if (chase_degree < 0)
        {
            chase_degree = -chase_degree;
        }
        if (chase_degree >= 100)
        {
            goto return_pad;
        }
        if (rand() % 5 != 0)
        {
            goto return_pad;
        }
        pad = 0x1040;
        goto return_pad;
    }

    if ((Me_THINK_C->motion->count & 0xf) != 0)
    {
        s32 raw_degree;
        s32 motion_degree;

        pad = Me_THINK_C->pad.data;
        if (Distance >= 2000)
        {
            goto return_pad;
        }
        raw_degree = Degree;
        motion_degree = (raw_degree >= 0) ? raw_degree : -raw_degree;
        if (motion_degree < 1000)
        {
            pad = 0x4000;
            goto return_pad;
        }
        if (motion_degree < 1501)
        {
            goto return_pad;
        }
        pad = 0x1000;
        goto return_pad;
    }

    if (Distance >= 5001)
    {
        Humanoid *me;

        Me_THINK_C->actmode = 0;
        me = Me_THINK_C;
        Me_THINK_C->chase[1] = 0;
        me->chase[0] = 0;
        ItemUse();
        return 0;
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

    if ((u32)(Distance - 1001) < 1999)
    {
        s32 attack_degree;

        do
        {
            attack_degree = Degree;
            if (attack_degree < 0)
            {
                attack_degree = -attack_degree;
            }
            if (attack_degree >= 1200)
            {
                break;
            }
            if (rand() % (EngageLevel + 1) != 0)
            {
                break;
            }
            if (GameClock > AttackActionCount)
            {
                AttackActionCount = GameClock + EngageLevel * 10;
                if (rand() % 3 == 0)
                {
                    pad = 0x4000;
                }
                return pad | 0x80;
            }
        } while (0);
    }

    {
        s32 degree;

        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }

        if (degree >= 1001)
        {
            goto close_range;
        }
        if (Distance >= 2000)
        {
            goto normal_range;
        }

close_range:
        if (Distance < 1000)
        {
            switch (rand() % 4)
            {
            case 0:
                pad = 0x4040;
                goto return_pad;
            case 1:
                pad = 0xa0;
                goto return_pad;
            case 2:
                pad = SetCommand(&Me_THINK_C->pad, 2);
                goto return_pad;
            case 3:
                pad |= 0x80;
                goto return_pad;
            default:
                goto return_pad;
            }
        }
        pad |= 0x4000;
        goto return_pad;

normal_range:
        if (Distance < 3001)
        {
            goto near_range;
        }

        pad |= 0x1000;
        if (Distance < 4001)
        {
            goto return_pad;
        }
        if ((rand() & 1) != 0)
        {
            pad = SetCommand(&Me_THINK_C->pad, 1);
            goto return_pad;
        }

        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree < 500)
        {
            pad = SetCommand(&Me_THINK_C->pad, 0x21);
            goto return_pad;
        }
        ItemUse();
        goto return_pad;

near_range:
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
        goto return_pad;
    }

return_pad:
    return pad;
}

// triage: HARD — 361 insns, mul/div, 4 callees, ~0.05 to AttackAnimal
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short AttackGeneral(void)
//
// {
//   Humanoid *pHVar1;
//   short sVar2;
//   int iVar3;
//   uint uVar4;
//   int iVar5;
//   ushort uVar6;
//
//   uVar6 = 0;
//   if (Me_THINK_C->status != 7) {
//     if (Me_THINK_C->status == 9) {
//       return 0;
//     }
//     if (Me_THINK_C->motion->mid == 0x500) {
//       iVar3 = rand();
//       iVar5 = EngageLevel + 1;
//       if (iVar5 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar5 == -1) && (iVar3 == -0x80000000)) {
//         trap(0x1800);
//       }
//       return (ushort)(iVar3 % iVar5 != 0) << 0xe;
//     }
//     if (Me_THINK_C->actmode == '\0') {
//       sVar2 = ChasetoTarget(3000);
//       if ((sVar2 == 0) || ((Attrib & 0x4000U) != 0)) {
//         Me_THINK_C->actmode = '\x01';
//       }
//       if (Distance < 0x1389) {
//         return sVar2;
//       }
//       iVar3 = (int)Degree;
//       if (iVar3 < 0) {
//         iVar3 = -iVar3;
//       }
//       if (99 < iVar3) {
//         return sVar2;
//       }
//       iVar3 = rand();
//       if (iVar3 != (iVar3 / 5) * 5) {
//         return sVar2;
//       }
//       return 0x1040;
//     }
//     if ((Me_THINK_C->motion->count & 0xfU) != 0) {
//       uVar6 = (Me_THINK_C->pad).data;
//       if (1999 < Distance) {
//         return uVar6;
//       }
//       iVar3 = (int)Degree;
//       if (iVar3 < 0) {
//         iVar3 = -iVar3;
//       }
//       if (iVar3 < 1000) {
//         return 0x4000;
//       }
//       if (iVar3 < 0x5dd) {
//         return uVar6;
//       }
//       return 0x1000;
//     }
//     if (5000 < Distance) {
//       Me_THINK_C->actmode = '\0';
//       pHVar1 = Me_THINK_C;
//       Me_THINK_C->chase[1] = 0;
//       pHVar1->chase[0] = 0;
//       ItemUse();
//       return 0;
//     }
//     if ((Attrib & 0x400U) != 0) {
//       Me_THINK_C->actmode = '\0';
//     }
//     if (Degree < 0x1f5) {
//       if (Degree < -500) {
//         uVar6 = 0x8000;
//       }
//     }
//     else {
//       uVar6 = 0x2000;
//     }
//     if (Distance - 0x3e9U < 1999) {
//       iVar3 = (int)Degree;
//       if (iVar3 < 0) {
//         iVar3 = -iVar3;
//       }
//       if (iVar3 < 0x4b0) {
//         iVar3 = rand();
//         iVar5 = EngageLevel + 1;
//         if (iVar5 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar5 == -1) && (iVar3 == -0x80000000)) {
//           trap(0x1800);
//         }
//         if ((iVar3 % iVar5 == 0) && (AttackActionCount < GameClock)) {
//           AttackActionCount = GameClock + EngageLevel * 10;
//           iVar3 = rand();
//           if (iVar3 == (iVar3 / 3) * 3) {
//             uVar6 = 0x4000;
//           }
//           return uVar6 | 0x80;
//         }
//       }
//     }
//     iVar3 = (int)Degree;
//     if (iVar3 < 0) {
//       iVar3 = -iVar3;
//     }
//     if ((iVar3 < 0x3e9) && (1999 < Distance)) {
//       if (Distance < 0xbb9) {
//         uVar4 = rand();
//         if ((uVar4 & 1) == 0) {
//           return uVar6;
//         }
//         if (Degree < 0x65) {
//           sVar2 = 3;
//           if (-0x65 < Degree) {
//             return uVar6;
//           }
//         }
//         else {
//           sVar2 = 4;
//         }
//       }
//       else {
//         if (Distance < 0xfa1) {
//           return uVar6 | 0x1000;
//         }
//         uVar4 = rand();
//         sVar2 = 1;
//         if ((uVar4 & 1) == 0) {
//           iVar3 = (int)Degree;
//           if (iVar3 < 0) {
//             iVar3 = -iVar3;
//           }
//           sVar2 = 0x21;
//           if (499 < iVar3) {
//             ItemUse();
//             return uVar6 | 0x1000;
//           }
//         }
//       }
//     }
//     else {
//       if (999 < Distance) {
//         return uVar6 | 0x4000;
//       }
//       iVar5 = rand();
//       iVar3 = iVar5;
//       if (iVar5 < 0) {
//         iVar3 = iVar5 + 3;
//       }
//       iVar5 = iVar5 + (iVar3 >> 2) * -4;
//       if (iVar5 == 1) {
//         return 0xa0;
//       }
//       if (iVar5 < 2) {
//         if (iVar5 != 0) {
//           return uVar6;
//         }
//         return 0x4040;
//       }
//       if (iVar5 != 2) {
//         if (iVar5 != 3) {
//           return uVar6;
//         }
//         return uVar6 | 0x80;
//       }
//       sVar2 = 2;
//     }
//     sVar2 = SetCommand(&Me_THINK_C->pad,sVar2);
//     return sVar2;
//   }
//   if (Me_THINK_C->motion->count != BattleDB[Me_THINK_C->warid].contfrm) {
//     return 0;
//   }
//   if (Distance < 2000) {
//     iVar3 = (int)Degree;
//     if (iVar3 < 0) {
//       iVar3 = -iVar3;
//     }
//     if (iVar3 < 500) goto LAB_8002e468;
//   }
//   iVar3 = rand();
//   iVar5 = EngageLevel + 1;
//   if (iVar5 == 0) {
//     trap(0x1c00);
//   }
//   if ((iVar5 == -1) && (iVar3 == -0x80000000)) {
//     trap(0x1800);
//   }
//   if (iVar3 % iVar5 != 0) {
//     return 0;
//   }
// LAB_8002e468:
//   if (Degree < 0x12d) {
//     if (-0x12d < Degree) {
//       return 0x80;
//     }
//     uVar6 = 0x8000;
//   }
//   else {
//     uVar6 = 0x2000;
//   }
//   return uVar6 | 0x80;
// }
