#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void SetLightningI(struct VECTOR *start, struct VECTOR *end, int gen, short r, int g, int b);
 *     EFFECT.C:1500, 60 src lines, frame 152 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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
 *     param $s6       struct VECTOR * start
 *     param $s7       struct VECTOR * end
 *     param stack+4294967216 int gen
 *     param stack+4294967224 short r
 *     param stack+16  int g
 *     param stack+20  int b
 *     stack sp+88     short g
 *     stack sp+96     short b
 *     reg   $s0       long lcount
 *     reg   $s4       int i
 *     stack sp+24     struct SVECTOR scr
 *     stack sp+32     struct SVECTOR oldscr
 *     reg   $s1       int x
 *     reg   $s2       int y
 *     reg   $s3       int z
 *     stack sp+40     struct GsLINE line
 *     reg   $v0       long x
 *     reg   $t1       long y
 *     reg   $t2       long z
 *     reg   $s6       struct VECTOR * v1
 *     reg   $s7       struct VECTOR * v2
 *     reg   $a1       long dz
 *     reg   $a3       long dy
 *     reg   $t0       long dx
 *     stack sp+56     struct VECTOR sv
 *     reg   $s1       long x
 *     reg   $s2       long y
 *     reg   $s3       long z
 *     reg   $fp       struct SVECTOR * scr
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetLightningI", SetLightningI);
#else

typedef struct
{
    u32 attribute;
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
    u8 r;
    u8 g;
    u8 b;
} LightningLine;

typedef struct
{
    s32 vpx;
    s32 vpy;
    s32 vpz;
    s32 vrx;
    s32 vry;
    s32 vrz;
} LightningViewInfo;

extern LightningViewInfo ViewInfo;
extern MATRIX GsWSMATRIX;
extern GsOT *OTablePt;

extern void SetTransMatrix(MATRIX *matrix);
extern void SetRotMatrix(MATRIX *matrix);
extern s32 RotTransPers(SVECTOR *vector, s32 *screen, void *p, void *flag);
extern s32 SquareRoot0(s32 value);
extern s32 abs(s32 value);
extern int rand(void);
extern void *memset(void *dst, int value, u32 size);
extern void GsSortLine(LightningLine *line, GsOT *ot, u16 priority);

void SetLightningI(VECTOR *start, VECTOR *end, int gen, short r, short g, short b)
{
    SVECTOR scr;
    SVECTOR oldscr;
    LightningLine line;
    VECTOR sv;
    int next_gen;
    short local_r;
    short local_g;
    short local_b;
    VECTOR *svp;
    SVECTOR *scrp;
    long distance;
    long lcount;
    int i;
    int x;
    int y;
    int z;

    local_r = r;
    next_gen = gen - 1;
    local_g = g;
    local_b = b;
    if (gen != 0)
    {
        *(s32 *)0x1F800014 = 0;
        *(s32 *)0x1F800018 = 0;
        *(s32 *)0x1F80001C = 0;
        SetTransMatrix((MATRIX *)0x1F800000);
        SetRotMatrix(&GsWSMATRIX);
        {
            long initial_x;
            long initial_y;
            long initial_z;

            initial_x = start->vx;
            initial_y = start->vy;
            initial_z = start->vz;
            *(s16 *)0x1F800080 = initial_x - (s16)ViewInfo.vpx;
            *(s16 *)0x1F800082 = initial_y - (s16)ViewInfo.vpy;
            *(s16 *)0x1F800084 = initial_z - (s16)ViewInfo.vpz;
        }
        oldscr.vz = (s16)RotTransPers((SVECTOR *)0x1F800080, (s32 *)&oldscr,
                                      (void *)0x1F800000, (void *)0x1F800010);

        line.attribute = 0x50000000;
        line.r = local_r;
        line.g = local_g;
        line.b = local_b;

        {
            long dx;
            long dy;
            long dz;
            int large;

            large = 0;
            dx = start->vx - end->vx;
            dy = start->vy - end->vy;
            dz = start->vz - end->vz;
            if (abs(dx) > 0x1000 || abs(dy) > 0x1000 || abs(dz) > 0x1000)
            {
                large = 1;
            }

            if (large)
            {
                dx /= 0x100;
                dy /= 0x100;
                dz /= 0x100;
                distance = SquareRoot0(dx * dx + dy * dy + dz * dz) << 8;
            }
            else
            {
                distance = SquareRoot0(dx * dx + dy * dy + dz * dz);
            }
        }

        lcount = distance / 200;
        i = 1;
        if (lcount > 0)
        {
            svp = &sv;
            scrp = &scr;
            while (1)
            {
                if (i >= lcount)
                {
                    break;
                }

                x = ((end->vx - start->vx) * i) / lcount + start->vx;
                y = ((end->vy - start->vy) * i) / lcount + start->vy;
                z = ((end->vz - start->vz) * i) / lcount + start->vz;
                x += -0x50 + rand() % 0xa0;
                y += -0x50 + rand() % 0xa0;
                z += -0x50 + rand() % 0xa0;

                if ((rand() & 2) == 0)
                {
                    memset(svp, 0, sizeof(VECTOR));
                    sv.vx = x;
                    sv.vy = y;
                    sv.vz = z;
                    SetLightningI(svp, end, next_gen, local_r, local_g, local_b);
                }

                *(s16 *)0x1F800080 = x - (s16)ViewInfo.vpx;
                *(s16 *)0x1F800082 = y - (s16)ViewInfo.vpy;
                *(s16 *)0x1F800084 = z - (s16)ViewInfo.vpz;
                scrp->vz = (s16)RotTransPers((SVECTOR *)0x1F800080, (s32 *)scrp,
                                              (void *)0x1F800000, (void *)0x1F800010);

                {
                    s32 depth;

                    depth = (s32)(u16)scr.vz << 16;
                    if (depth > 0 && oldscr.vz > 0)
                    {
                        int priority;

                        line.y0 = oldscr.vy;
                        do
                        {
                            priority = depth >> 18;
                        } while (0);
                        line.x0 = oldscr.vx;
                        line.x1 = scr.vx;
                        line.y1 = scr.vy;
                        if (priority < 0)
                        {
                            priority = 0;
                        }
                        else if (priority >= 0x4e2)
                        {
                            priority = 0x4e1;
                        }
                        GsSortLine(&line, OTablePt, (u16)priority);
                    }
                }
                oldscr = scr;
                i++;
            }
        }
    }
}

#endif

// triage: HARD — 346 insns, mul/div, 9 callees, ~0.04 to GetVectorRotation
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SetLightningI(VECTOR *start,VECTOR *end,int gen,short r,int g,int b)
//
// {
//   int iVar1;
//   bool bVar2;
//   long lVar3;
//   int iVar4;
//   int iVar5;
//   uint uVar6;
//   int iVar7;
//   int iVar8;
//   int iVar9;
//   int iVar10;
//   int iVar11;
//   int iVar12;
//   undefined4 local_80;
//   undefined4 local_7c;
//   undefined4 local_78;
//   undefined4 local_74;
//   GsLINE local_70;
//   VECTOR local_60;
//   int local_50;
//   short local_48;
//   short local_40;
//   short local_38;
//   VECTOR *local_30;
//
//   local_50 = gen + -1;
//   local_40 = (short)g;
//   local_38 = (short)b;
//   if (gen != 0) {
//     Scratchpad[0x14] = 0;
//     Scratchpad[0x15] = 0;
//     Scratchpad[0x16] = 0;
//     Scratchpad[0x17] = 0;
//     Scratchpad[0x18] = 0;
//     Scratchpad[0x19] = 0;
//     Scratchpad[0x1a] = 0;
//     Scratchpad[0x1b] = 0;
//     Scratchpad[0x1c] = 0;
//     Scratchpad[0x1d] = 0;
//     Scratchpad[0x1e] = 0;
//     Scratchpad[0x1f] = 0;
//     local_48 = r;
//     SetTransMatrix((MATRIX *)Scratchpad);
//     SetRotMatrix(&GsWSMATRIX);
//     Scratchpad._128_2_ = (short)start->vx - (short)ViewInfo.vpx;
//     Scratchpad._130_2_ = (short)start->vy - (short)ViewInfo.vpy;
//     Scratchpad._132_2_ = (short)start->vz - (short)ViewInfo.vpz;
//     lVar3 = RotTransPers((SVECTOR *)(Scratchpad + 0x80),&local_78,(long *)Scratchpad,
//                          (long *)(Scratchpad + 0x10));
//     local_74 = CONCAT22(local_74._2_2_,(short)lVar3);
//     bVar2 = false;
//     local_70.attribute = 0x50000000;
//     local_70.r = (uchar)local_48;
//     local_70.g = (uchar)local_40;
//     local_70.b = (uchar)local_38;
//     iVar11 = start->vx - end->vx;
//     iVar9 = start->vy - end->vy;
//     iVar10 = start->vz - end->vz;
//     iVar4 = abs(iVar11);
//     if (((0x1000 < iVar4) || (iVar4 = abs(iVar9), 0x1000 < iVar4)) ||
//        (iVar4 = abs(iVar10), 0x1000 < iVar4)) {
//       bVar2 = true;
//     }
//     if (bVar2) {
//       if (iVar11 < 0) {
//         iVar11 = iVar11 + 0xff;
//       }
//       if (iVar9 < 0) {
//         iVar9 = iVar9 + 0xff;
//       }
//       if (iVar10 < 0) {
//         iVar10 = iVar10 + 0xff;
//       }
//       lVar3 = SquareRoot0((iVar11 >> 8) * (iVar11 >> 8) + (iVar9 >> 8) * (iVar9 >> 8) +
//                           (iVar10 >> 8) * (iVar10 >> 8));
//       lVar3 = lVar3 << 8;
//     }
//     else {
//       lVar3 = SquareRoot0(iVar11 * iVar11 + iVar9 * iVar9 + iVar10 * iVar10);
//     }
//     iVar4 = lVar3 / 200;
//     iVar11 = 1;
//     if (0 < iVar4) {
//       local_30 = &local_60;
//       for (; iVar11 < iVar4; iVar11 = iVar11 + 1) {
//         iVar10 = start->vx;
//         iVar9 = (end->vx - iVar10) * iVar11;
//         if (iVar4 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar4 == -1) && (iVar9 == -0x80000000)) {
//           trap(0x1800);
//         }
//         iVar8 = start->vy;
//         iVar12 = (end->vy - iVar8) * iVar11;
//         if (iVar4 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar4 == -1) && (iVar12 == -0x80000000)) {
//           trap(0x1800);
//         }
//         iVar7 = start->vz;
//         iVar1 = (end->vz - iVar7) * iVar11;
//         if (iVar4 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar4 == -1) && (iVar1 == -0x80000000)) {
//           trap(0x1800);
//         }
//         iVar5 = rand();
//         iVar10 = iVar9 / iVar4 + iVar10 + -0x50 + iVar5 % 0xa0;
//         iVar9 = rand();
//         iVar12 = iVar12 / iVar4 + iVar8 + -0x50 + iVar9 % 0xa0;
//         iVar9 = rand();
//         iVar9 = iVar1 / iVar4 + iVar7 + -0x50 + iVar9 % 0xa0;
//         uVar6 = rand();
//         if ((uVar6 & 2) == 0) {
//           memset((uchar *)local_30,'\0',0x10);
//           local_60.vx = iVar10;
//           local_60.vy = iVar12;
//           local_60.vz = iVar9;
//           SetLightningI(local_30,end,local_50,local_48,(int)local_40,(int)local_38);
//         }
//         Scratchpad._128_2_ = (short)iVar10 - (short)ViewInfo.vpx;
//         Scratchpad._130_2_ = (short)iVar12 - (short)ViewInfo.vpy;
//         Scratchpad._132_2_ = (short)iVar9 - (short)ViewInfo.vpz;
//         lVar3 = RotTransPers((SVECTOR *)(Scratchpad + 0x80),&local_80,(long *)Scratchpad,
//                              (long *)(Scratchpad + 0x10));
//         local_7c = CONCAT22(local_7c._2_2_,(short)lVar3);
//         if ((0 < lVar3 << 0x10) && (0 < (short)local_74)) {
//           local_70.y0 = local_78._2_2_;
//           iVar9 = (lVar3 << 0x10) >> 0x12;
//           local_70.x0 = (short)local_78;
//           local_70.x1 = (short)local_80;
//           local_70.y1 = local_80._2_2_;
//           if (iVar9 < 0) {
//             iVar10 = 0;
//           }
//           else {
//             iVar10 = 0x4e1;
//             if (iVar9 < 0x4e2) {
//               iVar10 = iVar9;
//             }
//           }
//           GsSortLine(&local_70,OTablePt,(ushort)iVar10);
//         }
//         local_78 = local_80;
//         local_74 = local_7c;
//       }
//     }
//   }
//   return;
// }
