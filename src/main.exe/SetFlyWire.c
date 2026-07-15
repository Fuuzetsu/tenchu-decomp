#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int SetFlyWire(struct VECTOR *start, struct VECTOR *end);
 *     EFFECT.C:1384, 40 src lines, frame 48 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     param $a2       struct VECTOR * start
 *     param $a3       struct VECTOR * end
 *     reg   $s5       struct tag_EffectSlot * slot
 *     reg   $s3       struct FlyWireType * param
 *     reg   $s2       int dist
 *     reg   $a1       int i
 *     reg   $s3       struct VECTOR * v1
 *     reg   $a0       long dz
 *     reg   $a2       long dy
 *     reg   $t0       long dx
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

extern s32 SquareRoot0(s32 value);
extern long abs(long value);
extern void DrawFlyWire(TEffectSlot *ef);

int SetFlyWire(VECTOR *start, VECTOR *end)
{
    TEffectSlot *base;
    TEffectSlot *slot;
    TEffectSlot *ef;
    FlyWireType *param;
    int idx;
    int i;
    int dist;
    int result;

    idx = CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_;
    i = 0;
    base = EffectSlot;
    slot = base + idx;
loop:
    idx = idx + 1;
    slot = slot + 1;
    if (199 < idx)
    {
        slot = base;
        idx = 0;
    }
    i = i + 1;
    if (slot->proc == 0)
    {
        CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = idx + 1;
        if (199 < idx + 1)
        {
            CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = 0;
        }
        ef = slot;
        goto found;
    }
    if (199 < i)
    {
        ef = &dmy;
        goto found;
    }
    goto loop;

found:
    param = &ef->param.flywire;
    param->start = *start;
    param->end = *end;
    param->count = 0;
    param->mode = 0;

    {
        VECTOR *v1;
        long dz;
        long dy;
        long dx;
        int big;
        long v;
        long root;
        long scaled;
        long range;
        long base_x;
        long base_y;
        long base_z;
        long value_x;
        long value_y;
        long value_z;

        v1 = &param->end;
        dx = param->start.vx - v1->vx;
        dy = param->start.vy - v1->vy;
        dz = param->start.vz - v1->vz;

        big = 0;
        if (abs(dx) > 0x1000 || abs(dy) > 0x1000 || abs(dz) > 0x1000)
        {
            big = 1;
        }
        if (big)
        {
            v = dx;
            if (dx < 0)
            {
                v = dx + 0xff;
            }
            dx = v >> 8;
            do {
              v = dy;
            } while (0);
            if (dy < 0)
            {
                v = dy + 0xff;
            }
            dy = v >> 8;
            v = dz;
            if (dz < 0)
            {
                v = dz + 0xff;
            }
            dz = v >> 8;
            root = SquareRoot0(dx * dx + dy * dy + dz * dz) << 8;
        }
        else
        {
            root = SquareRoot0(dx * dx + dy * dy + dz * dz);
        }
        dist = root;

        param->NCenter.vx = (param->start.vx + param->end.vx) / 2;
        param->NCenter.vy = (param->start.vy + param->end.vy) / 2;
        param->NCenter.vz = (param->start.vz + param->end.vz) / 2;
        param->time = dist / 1000;

        scaled = dist;
        if (dist < 0)
        {
            scaled = dist + 0xf;
        }
        do {
            dist = scaled >> 4;
        } while (0);

        base_x = param->NCenter.vx;
        range = dist * 2;
        if (range > 0)
        {
            value_x = base_x + (rand() % range - dist);
        }
        else
        {
            value_x = base_x - dist;
        }
        param->center.vx = value_x;

        base_y = param->NCenter.vy;
        if (dist > 0)
        {
            value_y = base_y + (rand() % dist - dist);
        }
        else
        {
            value_y = base_y - dist;
        }
        param->center.vy = value_y;

        base_z = param->NCenter.vz;
        range = dist * 2;
        if (range > 0)
        {
            value_z = base_z + (rand() % range - dist);
        }
        else
        {
            value_z = base_z - dist;
        }
        param->center.vz = value_z;
    }

    if (param->time > 0)
    {
        ef->proc = (void (*)())DrawFlyWire;
        result = param->time + 5;
    }
    else
    {
        result = 0;
    }
    return result;
}

// triage: MEDIUM — 246 insns, mul/div, 1 loop, 3 callees, ~0.06 to AddItem2
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// int SetFlyWire(VECTOR *start,VECTOR *end)
//
// {
//   short sVar1;
//   bool bVar2;
//   int iVar3;
//   int iVar4;
//   int iVar5;
//   long lVar6;
//   long lVar7;
//   long lVar8;
//   int iVar9;
//   int iVar10;
//   tag_EffectSlot *ptVar11;
//
//   iVar5 = 0;
//   ptVar11 = EffectSlot + DAT_80097a3c;
//   iVar3 = DAT_80097a3c;
//   while( true ) {
//     iVar3 = iVar3 + 1;
//     ptVar11 = ptVar11 + 1;
//     if (199 < iVar3) {
//       iVar3 = 0;
//       ptVar11 = EffectSlot;
//     }
//     iVar5 = iVar5 + 1;
//     if (ptVar11->proc == (undefined **)0x0) break;
//     if (199 < iVar5) {
//       ptVar11 = &dmy;
// LAB_80036fa4:
//       bVar2 = false;
//       lVar6 = start->vy;
//       lVar7 = start->vz;
//       lVar8 = start->pad;
//       (ptVar11->param).blood.hint = (AreaNodeType *)start->vx;
//       (ptVar11->param).blood.px = lVar6;
//       (ptVar11->param).blood.py = lVar7;
//       (ptVar11->param).blood.pz = lVar8;
//       lVar6 = end->vy;
//       lVar7 = end->vz;
//       lVar8 = end->pad;
//       (ptVar11->param).blood.scale = end->vx;
//       (ptVar11->param).blood.rotate = lVar6;
//       (ptVar11->param).smoke.rotate = lVar7;
//       (ptVar11->param).smoke.scale = lVar8;
//       (ptVar11->param).flywire.count = 0;
//       (ptVar11->param).flywire.mode = '\0';
//       iVar5 = (int)(ptVar11->param).blood.hint - (ptVar11->param).blood.scale;
//       iVar9 = (ptVar11->param).blood.px - (ptVar11->param).blood.rotate;
//       iVar10 = (ptVar11->param).blood.py - (ptVar11->param).smoke.rotate;
//       iVar3 = abs(iVar5);
//       if (((0x1000 < iVar3) || (iVar3 = abs(iVar9), 0x1000 < iVar3)) ||
//          (iVar3 = abs(iVar10), 0x1000 < iVar3)) {
//         bVar2 = true;
//       }
//       if (bVar2) {
//         if (iVar5 < 0) {
//           iVar5 = iVar5 + 0xff;
//         }
//         if (iVar9 < 0) {
//           iVar9 = iVar9 + 0xff;
//         }
//         if (iVar10 < 0) {
//           iVar10 = iVar10 + 0xff;
//         }
//         lVar6 = SquareRoot0((iVar5 >> 8) * (iVar5 >> 8) + (iVar9 >> 8) * (iVar9 >> 8) +
//                             (iVar10 >> 8) * (iVar10 >> 8));
//         iVar3 = lVar6 << 8;
//       }
//       else {
//         iVar3 = SquareRoot0(iVar5 * iVar5 + iVar9 * iVar9 + iVar10 * iVar10);
//       }
//       iVar10 = (ptVar11->param).blood.rotate;
//       iVar9 = (ptVar11->param).blood.px;
//       (ptVar11->param).flywire.NCenter.vx =
//            ((int)&((ptVar11->param).blood.hint)->y + (ptVar11->param).blood.scale) / 2;
//       iVar5 = (ptVar11->param).blood.py;
//       iVar4 = (ptVar11->param).smoke.rotate;
//       (ptVar11->param).flywire.NCenter.vy = (iVar9 + iVar10) / 2;
//       (ptVar11->param).flywire.NCenter.vz = (iVar5 + iVar4) / 2;
//       (ptVar11->param).flywire.time = (short)(iVar3 / 1000);
//       if (iVar3 < 0) {
//         iVar3 = iVar3 + 0xf;
//       }
//       iVar3 = iVar3 >> 4;
//       iVar9 = (ptVar11->param).flywire.NCenter.vx;
//       iVar5 = iVar3 << 1;
//       if (iVar5 < 1) {
//         iVar5 = -iVar3;
//       }
//       else {
//         iVar10 = rand();
//         if (iVar5 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar5 == -1) && (iVar10 == -0x80000000)) {
//           trap(0x1800);
//         }
//         iVar5 = iVar10 % iVar5 - iVar3;
//       }
//       iVar10 = (ptVar11->param).flywire.NCenter.vy;
//       (ptVar11->param).flywire.center.vx = iVar9 + iVar5;
//       if (iVar3 < 1) {
//         iVar5 = -iVar3;
//       }
//       else {
//         iVar5 = rand();
//         if (iVar3 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar3 == -1) && (iVar5 == -0x80000000)) {
//           trap(0x1800);
//         }
//         iVar5 = iVar5 % iVar3 - iVar3;
//       }
//       iVar4 = (ptVar11->param).flywire.NCenter.vz;
//       iVar9 = iVar3 << 1;
//       (ptVar11->param).flywire.center.vy = iVar10 + iVar5;
//       if (iVar9 < 1) {
//         iVar3 = -iVar3;
//       }
//       else {
//         iVar5 = rand();
//         if (iVar9 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar9 == -1) && (iVar5 == -0x80000000)) {
//           trap(0x1800);
//         }
//         iVar3 = iVar5 % iVar9 - iVar3;
//       }
//       sVar1 = (ptVar11->param).flywire.time;
//       (ptVar11->param).flywire.center.vz = iVar4 + iVar3;
//       if (sVar1 < 1) {
//         iVar3 = 0;
//       }
//       else {
//         ptVar11->proc = (undefined **)DrawFlyWire;
//         iVar3 = (ptVar11->param).flywire.time + 5;
//       }
//       return iVar3;
//     }
//   }
//   DAT_80097a3c = iVar3 + 1;
//   if (199 < DAT_80097a3c) {
//     DAT_80097a3c = 0;
//   }
//   goto LAB_80036fa4;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// s32 SquareRoot0(s32);                               /* extern */
// s32 abs(s32, s32, ? **, s32);                       /* extern */
// s32 rand(s32, ?, s32);                              /* extern */
// extern s32 CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_;
// extern ? DrawFlyWire;
// extern ? *EffectSlot;
// extern ? *dmy;
//
// s32 SetFlyWire(void *arg0, void *arg1) {
//     ? **temp_v0_2;
//     ? **var_a2;
//     ? **var_s3;
//     ? **var_s4;
//     s32 temp_a0;
//     s32 temp_a1;
//     s32 temp_s0;
//     s32 temp_s0_2;
//     s32 temp_s0_3;
//     s32 temp_s0_4;
//     s32 temp_s0_5;
//     s32 temp_s1;
//     s32 temp_s1_2;
//     s32 temp_s1_3;
//     s32 temp_s2;
//     s32 temp_s2_2;
//     s32 temp_s2_3;
//     s32 temp_s2_4;
//     s32 temp_v0;
//     s32 var_a2_2;
//     s32 var_a3;
//     s32 var_s5;
//     s32 var_v0;
//     s32 var_v0_2;
//     s32 var_v0_3;
//     s32 var_v0_4;
//     s32 var_v0_5;
//     s32 var_v0_6;
//     s32 var_v1;
//     s32 var_v1_2;
//     u32 temp_v1;
//     void *temp_v0_3;
//
//     var_a3 = 0;
//     var_a2 = (CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ * 0x4C) + &EffectSlot;
//     var_v1 = CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ + 1;
// loop_1:
//     var_a2 += 0x4C;
//     if (var_v1 >= 0xC8) {
//         var_a2 = &EffectSlot;
//         var_v1 = 0;
//     }
//     var_a3 += 1;
//     if (*var_a2 == NULL) {
//         temp_v0 = var_v1 + 1;
//         CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = temp_v0;
//         var_s4 = var_a2;
//         if (temp_v0 >= 0xC8) {
//             CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = 0;
//             var_s3 = var_s4 + 4;
//         } else {
//             goto block_8;
//         }
//     } else {
//         var_v1 += 1;
//         if (var_a3 >= 0xC8) {
//             var_s4 = &dmy;
// block_8:
//             var_s3 = var_s4 + 4;
//         } else {
//             goto loop_1;
//         }
//     }
//     var_s5 = 0;
//     var_s4->unk4 = (s32) arg0->unk0;
//     var_s4->unk8 = (s32) arg0->unk4;
//     var_s4->unkC = (s32) arg0->unk8;
//     var_s4->unk10 = (s32) arg0->unkC;
//     var_s4->unk14 = (s32) arg1->unk0;
//     var_s4->unk18 = (s32) arg1->unk4;
//     var_s4->unk1C = (s32) arg1->unk8;
//     var_s4->unk20 = (s32) arg1->unkC;
//     var_s3->unk40 = 0;
//     var_s3->unk44 = 0;
//     temp_a1 = var_s3->unk4;
//     temp_s2 = var_s4->unk4 - var_s4->unk14;
//     temp_v0_2 = var_s4 + 0x14;
//     temp_s0 = temp_a1 - temp_v0_2->unk4;
//     temp_s1 = var_s3->unk8 - temp_v0_2->unk8;
//     if ((abs(temp_s2, temp_a1, var_a2, var_a3) >= 0x1001) || (abs(temp_s0) >= 0x1001) || (abs(temp_s1) >= 0x1001)) {
//         var_s5 = 1;
//     }
//     if (var_s5 != 0) {
//         var_v0 = temp_s2;
//         if (temp_s2 < 0) {
//             var_v0 = temp_s2 + 0xFF;
//         }
//         temp_s2_2 = var_v0 >> 8;
//         var_v0_2 = temp_s0;
//         if (temp_s0 < 0) {
//             var_v0_2 = temp_s0 + 0xFF;
//         }
//         temp_s0_2 = var_v0_2 >> 8;
//         var_v0_3 = temp_s1;
//         if (temp_s1 < 0) {
//             var_v0_3 = temp_s1 + 0xFF;
//         }
//         temp_s1_2 = var_v0_3 >> 8;
//         var_v0_4 = SquareRoot0((temp_s2_2 * temp_s2_2) + (temp_s0_2 * temp_s0_2) + (temp_s1_2 * temp_s1_2)) << 8;
//     } else {
//         var_v0_4 = SquareRoot0((temp_s2 * temp_s2) + (temp_s0 * temp_s0) + (temp_s1 * temp_s1));
//     }
//     var_a2_2 = var_v0_4;
//     temp_v0_3 = var_s3->unk0 + var_s3->unk10;
//     var_s3->unk30 = (s32) ((s32) (temp_v0_3 + ((u32) temp_v0_3 >> 0x1F)) >> 1);
//     temp_v1 = var_s3->unk4 + var_s3->unk14;
//     temp_a0 = var_s3->unk18;
//     var_s3->unk34 = (s32) ((s32) (temp_v1 + (temp_v1 >> 0x1F)) >> 1);
//     var_s3->unk38 = (s32) ((s32) (var_s3->unk8 + temp_a0) / 2);
//     var_s3->unk42 = (s16) (var_v0_4 / 1000);
//     if (var_v0_4 < 0) {
//         var_a2_2 = var_v0_4 + 0xF;
//     }
//     temp_s1_3 = var_a2_2 >> 4;
//     temp_s2_3 = var_s3->unk30;
//     temp_s0_3 = temp_s1_3 * 2;
//     if (temp_s0_3 > 0) {
//         var_v0_5 = temp_s2_3 + ((rand(temp_a0, 0x10624DD3, var_a2_2) % temp_s0_3) - temp_s1_3);
//     } else {
//         var_v0_5 = temp_s2_3 - temp_s1_3;
//     }
//     temp_s0_4 = var_s3->unk34;
//     var_s3->unk20 = var_v0_5;
//     if (temp_s1_3 > 0) {
//         var_v0_6 = temp_s0_4 + ((rand() % temp_s1_3) - temp_s1_3);
//     } else {
//         var_v0_6 = temp_s0_4 - temp_s1_3;
//     }
//     temp_s2_4 = var_s3->unk38;
//     temp_s0_5 = temp_s1_3 * 2;
//     var_s3->unk24 = var_v0_6;
//     if (temp_s0_5 > 0) {
//         var_v1_2 = temp_s2_4 + ((rand() % temp_s0_5) - temp_s1_3);
//     } else {
//         var_v1_2 = temp_s2_4 - temp_s1_3;
//     }
//     var_s3->unk28 = var_v1_2;
//     if (var_s3->unk42 > 0) {
//         var_s4->unk0 = &DrawFlyWire;
//         return var_s3->unk42 + 5;
//     }
//     return 0;
// }
