#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ArrangeLocalMatrix(struct ModelType *model, struct MATRIX *t);
 *     ITEM.C:3328, 38 src lines, frame 72 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $a0       struct ModelType * model
 *     param $s3       struct MATRIX * t
 *     stack sp+16     struct MATRIX m
 *     reg   $a1       int i
 *     reg   $t4       int j
 *     reg   $t5       int k
 *     reg   $s1       long det
 *     reg   $a3       long t
 *     reg   $t1       long u
 * END PSX.SYM */

/*
 * MATCH.
 *
 * This is a fixed-point Gauss-Jordan pass over the local 3x3 matrix.  Each
 * pivot row is normalised, the pivot column is eliminated from the other
 * rows, and a determinant-like scale check decides whether to compose and
 * publish the result.
 *
 * Matching notes:
 *  - The outer `i` loop and middle `j` loop are explicit top-tested
 *    `while (1)` loops.  The two `k` loops stay ordinary bottom-tested loops;
 *    that accounts for the target's two extra guard/jump pairs.
 *  - The first row-normalisation loop deliberately reuses `k`, as does the
 *    deepest elimination loop.  Retail therefore gives both counters $a2,
 *    while the middle `j` loop stays in $t4.  Using `j` for both adjacent
 *    loops preserves the values but rotates nine caller-saved registers.
 *  - Spelling the deepest test as `k != i` puts the multiply/subtract path
 *    first and the variable division later.  The opposite equivalent test is
 *    one instruction short because its jump delay slot hides the divide
 *    result's hazard gap.
 *  - The final whole-MATRIX assignment expands to the target's eight-word
 *    copy.  Variable divisions require maspsx's `--expand-div` compatibility
 *    pass to reproduce ASPSX's guarded `div` sequences.
 */


void ArrangeLocalMatrix(ModelType *model, MATRIX *matrix)
{
    enum
    {
        n = 3
    };
    MATRIX m;
    s32 i;
    s32 j;
    s32 k;
    s32 det;

    GsGetLw(&model->locate, &m);
    det = 0x1000;

    i = 0;
    while (1)
    {
        s32 t;

        if (!(i < n))
        {
            break;
        }
        t = m.m[i][i];
        if (t == 0)
        {
            t = 0x1000;
        }
        det = det * t / 0x1000;

        for (k = 0; k < n; k++)
        {
            m.m[i][k] = m.m[i][k] * 0x1000 / t;
        }
        m.m[i][i] = 0x1000000 / t;

        j = 0;
        while (1)
        {
            if (!(j < n))
            {
                break;
            }
            if (j != i)
            {
                s32 u;

                u = m.m[j][i];
                for (k = 0; k < n; k++)
                {
                    if (k != i)
                    {
                        m.m[j][k] -= m.m[i][k] * u / 0x1000;
                    }
                    else
                    {
                        m.m[j][i] = (-u * 0x1000) / t;
                    }
                }
            }
            j++;
        }
        i++;
    }

    if ((u32)(det - 0x800) < 0x801)
    {
        MulMatrix(&m, matrix);
        *matrix = m;
    }
}

// triage: MEDIUM — 159 insns, mul/div, 2 loop, 2 callees, ~0.08 to AdtFntOpen
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Removing unreachable block (ram,0x80047a1c) */
//
// void ArrangeLocalMatrix(ModelType *model,MATRIX *t)
//
// {
//   int iVar1;
//   int iVar2;
//   int iVar3;
//   short *psVar4;
//   short *psVar5;
//   int iVar6;
//   int iVar7;
//   int iVar8;
//   int iVar9;
//   int iVar10;
//   int iVar11;
//   int iVar12;
//   int iVar13;
//   int iVar14;
//   int iVar15;
//   int iVar16;
//   MATRIX *pMVar17;
//   MATRIX local_50;
//   int local_30;
//
//   GsGetLw(&model->locate,&local_50);
//   iVar15 = 0x1000;
//   iVar13 = 0;
//   iVar14 = 0;
//   pMVar17 = &local_50;
//   for (iVar16 = 0; iVar16 < 3; iVar16 = iVar16 + 1) {
//     iVar10 = (int)pMVar17->m[0][0];
//     iVar1 = iVar15 * iVar10;
//     if (iVar10 == 0) {
//       iVar10 = 0x1000;
//       iVar1 = iVar15 * 0x1000;
//     }
//     if (iVar1 < 0) {
//       iVar1 = iVar1 + 0xfff;
//     }
//     iVar15 = iVar1 >> 0xc;
//     iVar7 = 0;
//     iVar1 = iVar13;
//     do {
//       psVar4 = (short *)((int)local_50.m[0] + iVar1);
//       iVar2 = (int)*psVar4 << 0xc;
//       if (iVar10 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar10 == -1) && (iVar2 == -0x80000000)) {
//         trap(0x1800);
//       }
//       iVar7 = iVar7 + 1;
//       *psVar4 = (short)(iVar2 / iVar10);
//       iVar1 = iVar1 + 2;
//     } while (iVar7 < 3);
//     if (iVar10 == 0) {
//       trap(0x1c00);
//     }
//     iVar2 = 0;
//     local_30 = iVar13;
//     pMVar17->m[0][0] = (short)(0x1000000 / iVar10);
//     iVar1 = iVar14;
//     for (iVar7 = 0; iVar7 < 3; iVar7 = iVar7 + 1) {
//       psVar4 = (short *)((int)local_50.m[0] + iVar1);
//       if (iVar7 != iVar16) {
//         iVar8 = 0;
//         iVar12 = (int)*psVar4;
//         iVar11 = iVar12 * -0x1000;
//         iVar6 = iVar2;
//         iVar9 = local_30;
//         do {
//           if (iVar8 == iVar16) {
//             if (iVar10 == 0) {
//               trap(0x1c00);
//             }
//             if ((iVar10 == -1) && (iVar11 == -0x80000000)) {
//               trap(0x1800);
//             }
//             *psVar4 = (short)(iVar11 / iVar10);
//           }
//           else {
//             iVar3 = *(short *)((int)local_50.m[0] + iVar9) * iVar12;
//             psVar5 = (short *)((int)local_50.m[0] + iVar6);
//             if (iVar3 < 0) {
//               iVar3 = iVar3 + 0xfff;
//             }
//             *psVar5 = *psVar5 - (short)(iVar3 >> 0xc);
//           }
//           iVar9 = iVar9 + 2;
//           iVar8 = iVar8 + 1;
//           iVar6 = iVar6 + 2;
//         } while (iVar8 < 3);
//       }
//       iVar1 = iVar1 + 6;
//       iVar2 = iVar2 + 6;
//     }
//     iVar13 = iVar13 + 6;
//     iVar14 = iVar14 + 2;
//     pMVar17 = (MATRIX *)(pMVar17->m[1] + 1);
//   }
//   if (iVar15 - 0x800U < 0x801) {
//     MulMatrix(&local_50,t);
//     *(undefined4 *)t->m[0] = local_50.m[0]._0_4_;
//     *(undefined4 *)(t->m[0] + 2) = local_50.m._4_4_;
//     *(undefined4 *)(t->m[1] + 1) = local_50.m[1]._2_4_;
//     *(undefined4 *)t->m[2] = local_50.m[2]._0_4_;
//     *(undefined4 *)(t->m[2] + 2) = local_50._16_4_;
//     t->t[0] = local_50.t[0];
//     t->t[1] = local_50.t[1];
//     t->t[2] = local_50.t[2];
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? GsGetLw(s32 *);                                   /* extern */
// ? MulMatrix(s32 *, void *);                         /* extern */
//
// void ArrangeLocalMatrix(void *arg1) {
//     s32 sp10;
//     s32 sp30;
//     s16 *temp_a0;
//     s16 *temp_v0;
//     s16 temp_t2;
//     s16 var_t0;
//     s32 *var_t9;
//     s32 var_a1;
//     s32 var_a1_2;
//     s32 var_a2;
//     s32 var_a2_2;
//     s32 var_a3;
//     s32 var_lo;
//     s32 var_s0;
//     s32 var_s2;
//     s32 var_s3;
//     s32 var_t4;
//     s32 var_t6;
//     s32 var_t7;
//     s32 var_t8;
//     s32 var_v0;
//     u16 *temp_a0_2;
//
//     GsGetLw(&sp10);
//     var_s3 = 0x1000;
//     var_t8 = 0;
//     var_s0 = 0;
//     var_s2 = 0;
//     var_t9 = &sp10;
// loop_1:
//     if (var_t8 < 3) {
//         var_t0 = *var_t9;
//         var_lo = var_s3 * var_t0;
//         if (var_t0 == 0) {
//             var_t0 = 0x1000;
//             var_lo = var_s3 * 0x1000;
//         }
//         var_s3 = var_lo >> 0xC;
//         if (var_lo < 0) {
//             var_s3 = (s32) (var_lo + 0xFFF) >> 0xC;
//         }
//         var_a2 = 0;
//         var_a1 = var_s0;
//         do {
//             temp_a0 = &sp10 + var_a1;
//             var_a2 += 1;
//             *temp_a0 = (s16) ((s32) (*temp_a0 << 0xC) / var_t0);
//             var_a1 += 2;
//         } while (var_a2 < 3);
//         var_t4 = 0;
//         var_t7 = var_s2;
//         var_t6 = 0;
//         sp30 = var_s0;
//         *var_t9 = (s16) (0x01000000 / var_t0);
// loop_9:
//         if (var_t4 < 3) {
//             temp_v0 = &sp10 + var_t7;
//             if (var_t4 != var_t8) {
//                 var_a2_2 = 0;
//                 var_a1_2 = var_t6;
//                 temp_t2 = *temp_v0;
//                 var_a3 = sp30;
//                 do {
//                     if (var_a2_2 != var_t8) {
//                         var_v0 = *(&sp10 + var_a3) * temp_t2;
//                         temp_a0_2 = &sp10 + var_a1_2;
//                         if (var_v0 < 0) {
//                             var_v0 += 0xFFF;
//                         }
//                         *temp_a0_2 -= var_v0 >> 0xC;
//                     } else {
//                         *temp_v0 = (s16) ((s32) (temp_t2 * -0x1000) / var_t0);
//                     }
//                     var_a3 += 2;
//                     var_a2_2 += 1;
//                     var_a1_2 += 2;
//                 } while (var_a2_2 < 3);
//             }
//             var_t7 += 6;
//             var_t6 += 6;
//             var_t4 += 1;
//             goto loop_9;
//         }
//         var_s0 += 6;
//         var_s2 += 2;
//         var_t9 += 8;
//         var_t8 += 1;
//         goto loop_1;
//     }
//     if ((u32) (var_s3 - 0x800) < 0x801U) {
//         MulMatrix(&sp10, arg1);
//         arg1->unk0 = sp10;
//         arg1->unk4 = sp14;
//         arg1->unk8 = sp18;
//         arg1->unkC = sp1C;
//         arg1->unk10 = sp20;
//         arg1->unk14 = sp24;
//         arg1->unk18 = sp28;
//         arg1->unk1C = sp2C;
//     }
// }
