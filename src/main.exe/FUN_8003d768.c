#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct tag_TItem items[30];
 *
 * PSX.SYM suggests this may be `RequestItem` (LOW confidence, ITEM.C) — NOT
 * adopted. Corroborate with `tools/callmatch.py --verify` before renaming.
 * END PSX.SYM */

/*
 * Draws the current map target and each live type-8 item owned by the camera
 * owner after rotating and scaling their X/Z coordinates through `area`.
 *
 * Matching notes:
 *  - Indexing `items` with the loop counter gives cc1 the target's single
 *    natural 0x58-byte induction pointer; a separately incremented item
 *    pointer was biased to `items + 0x10` and made the function four
 *    instructions too long.
 *  - The two X call arguments need distinct temporaries even though their
 *    expressions are identical and their live ranges do not overlap. Each
 *    temporary pulls the area[2] load/shift ahead of the Y rounding branch;
 *    sharing one source variable coalesces the regions and recolors both
 *    division chains, while inlining either argument schedules that region
 *    too late.
 *  - `--expand-div` is required for the retail signed-division overflow and
 *    divide-by-zero guards.
 */

extern void DrawTargetS(s32 x, s32 y, s32 z, s32 color);

void FUN_8003d768(s32 x, s32 z, s32 *area)
{
    s32 divisor;
    s32 sine;
    s32 first_draw_arg_x;
    s32 loop_draw_arg_x;
    s32 cosine;
    s32 draw_x;
    s32 draw_y;
    s32 i;

    divisor = area[0];
    if (divisor <= 0)
    {
        divisor = 1;
    }

    sine = rsin(area[1]);
    cosine = rcos(area[1]);

    draw_x = (x / divisor) * cosine + (z / divisor) * sine;
    if (draw_x < 0)
    {
        draw_x += 0xFFF;
    }
    first_draw_arg_x = (draw_x >> 12) + area[2];
    draw_y = (x / divisor) * sine - (z / divisor) * cosine;
    if (draw_y < 0)
    {
        draw_y += 0xFFF;
    }
    DrawTargetS(first_draw_arg_x, (draw_y >> 12) + area[3], 0, 0xC81414);

    i = 0;
    while (1)
    {
        if (!(i < 30))
        {
            break;
        }

        if (items[i].proc != 0 && items[i].type == 8 && items[i].owner == CamState.Owner)
        {
            draw_x = (items[i].locate->locate.coord.t[0] / divisor) * cosine +
                     (items[i].locate->locate.coord.t[2] / divisor) * sine;
            if (draw_x < 0)
            {
                draw_x += 0xFFF;
            }
            draw_y = (items[i].locate->locate.coord.t[0] / divisor) * sine -
                     (items[i].locate->locate.coord.t[2] / divisor) * cosine;
            loop_draw_arg_x = (draw_x >> 12) + area[2];
            if (draw_y < 0)
            {
                draw_y += 0xFFF;
            }
            DrawTargetS(loop_draw_arg_x, (draw_y >> 12) + area[3], 0, 0x1414C8);
        }

        i++;
    }
}

// triage: MEDIUM — 168 insns, mul/div, 3 callees, ~0.09 to GetVectorRotation
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation reference:
//
//
// void FUN_8003d768(int param_1,int param_2,int *param_3)
//
// {
//   int iVar1;
//   int iVar2;
//   int iVar3;
//   int iVar4;
//   tag_TItem *ptVar5;
//   int iVar6;
//   int iVar7;
//   int iVar8;
//
//   iVar6 = *param_3;
//   if (iVar6 < 1) {
//     iVar6 = 1;
//   }
//   iVar1 = rsin(param_3[1]);
//   iVar2 = rcos(param_3[1]);
//   if (iVar6 == 0) {
//     trap(0x1c00);
//   }
//   if ((iVar6 == -1) && (param_1 == -0x80000000)) {
//     trap(0x1800);
//   }
//   if (iVar6 == 0) {
//     trap(0x1c00);
//   }
//   if ((iVar6 == -1) && (param_2 == -0x80000000)) {
//     trap(0x1800);
//   }
//   iVar3 = (param_1 / iVar6) * iVar2 + (param_2 / iVar6) * iVar1;
//   if (iVar3 < 0) {
//     iVar3 = iVar3 + 0xfff;
//   }
//   iVar4 = (param_1 / iVar6) * iVar1 - (param_2 / iVar6) * iVar2;
//   if (iVar4 < 0) {
//     iVar4 = iVar4 + 0xfff;
//   }
//   DrawTargetS((iVar3 >> 0xc) + param_3[2],(iVar4 >> 0xc) + param_3[3],0,0xc81414);
//   ptVar5 = items;
//   for (iVar3 = 0; iVar3 < 0x1e; iVar3 = iVar3 + 1) {
//     if (((ptVar5->proc != (undefined **)0x0) && (ptVar5->type == ITEM_GOSHIKIMAI)) &&
//        (ptVar5->owner == CamState.Owner)) {
//       iVar4 = (ptVar5->locate->locate).coord.t[0];
//       iVar7 = iVar4 / iVar6;
//       if (iVar6 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar6 == -1) && (iVar4 == -0x80000000)) {
//         trap(0x1800);
//       }
//       iVar4 = (ptVar5->locate->locate).coord.t[2];
//       iVar8 = iVar4 / iVar6;
//       if (iVar6 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar6 == -1) && (iVar4 == -0x80000000)) {
//         trap(0x1800);
//       }
//       iVar4 = iVar7 * iVar2 + iVar8 * iVar1;
//       if (iVar4 < 0) {
//         iVar4 = iVar4 + 0xfff;
//       }
//       iVar7 = iVar7 * iVar1 - iVar8 * iVar2;
//       if (iVar7 < 0) {
//         iVar7 = iVar7 + 0xfff;
//       }
//       DrawTargetS((iVar4 >> 0xc) + param_3[2],(iVar7 >> 0xc) + param_3[3],0,0x1414c8);
//     }
//     ptVar5 = ptVar5 + 1;
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? DrawTargetS(s32, s32, ?, ?);                      /* extern */
// s32 rcos(s32);                                      /* extern */
// s32 rsin(s32);                                      /* extern */
// extern ? CamState;
// extern ? items;
//
// void FUN_8003d768(s32 arg0, s32 arg1, void *arg2) {
//     ? *var_s0;
//     s32 temp_lo;
//     s32 temp_lo_2;
//     s32 temp_lo_3;
//     s32 temp_lo_4;
//     s32 temp_s4;
//     s32 temp_v0;
//     s32 var_a1;
//     s32 var_a1_2;
//     s32 var_s1;
//     s32 var_s2;
//     s32 var_v0;
//     s32 var_v0_2;
//     void *temp_v1;
//
//     var_s1 = arg2->unk0;
//     if (var_s1 <= 0) {
//         var_s1 = 1;
//     }
//     temp_s4 = rsin(arg2->unk4);
//     temp_v0 = rcos(arg2->unk4);
//     temp_lo = arg0 / var_s1;
//     temp_lo_2 = arg1 / var_s1;
//     var_v0 = (temp_lo * temp_v0) + (temp_lo_2 * temp_s4);
//     if (var_v0 < 0) {
//         var_v0 += 0xFFF;
//     }
//     var_a1 = (temp_lo * temp_s4) - (temp_lo_2 * temp_v0);
//     if (var_a1 < 0) {
//         var_a1 += 0xFFF;
//     }
//     var_s2 = 0;
//     DrawTargetS((var_v0 >> 0xC) + arg2->unk8, (var_a1 >> 0xC) + arg2->unkC, 0, 0xC81414);
//     var_s0 = &items;
// loop_7:
//     if (var_s2 < 0x1E) {
//         if ((var_s0->unkC != 0) && (var_s0->unk8 == 8) && (var_s0->unk0 == CamState.unk10)) {
//             temp_v1 = var_s0->unk10;
//             temp_lo_3 = (s32) temp_v1->unk18 / var_s1;
//             temp_lo_4 = (s32) temp_v1->unk20 / var_s1;
//             var_v0_2 = (temp_lo_3 * temp_v0) + (temp_lo_4 * temp_s4);
//             if (var_v0_2 < 0) {
//                 var_v0_2 += 0xFFF;
//             }
//             var_a1_2 = (temp_lo_3 * temp_s4) - (temp_lo_4 * temp_v0);
//             if (var_a1_2 < 0) {
//                 var_a1_2 += 0xFFF;
//             }
//             DrawTargetS((var_v0_2 >> 0xC) + arg2->unk8, (var_a1_2 >> 0xC) + arg2->unkC, 0, 0x1414C8);
//         }
//         var_s0 += 0x58;
//         var_s2 += 1;
//         goto loop_7;
//     }
// }
