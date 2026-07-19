#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void SetLightningI(struct VECTOR *start, struct VECTOR *end, int gen, short r, int g, int b);
 *     EFFECT.C:1500, 60 src lines, frame 152 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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

/*
 * MATCHED: the source-level structure is the pair of small projection helpers
 * independently named by PSX.SYM in the same EFFECT.C translation unit:
 * PrepareGetScreenPositionS followed by GetScreenPositionS.  Keeping those
 * helpers inline preserves their scalar parameter identities at both call sites.
 * The debug-proven SVECTOR * type for the output is essential: screen->vz
 * retains the memory dependency that schedules the result store before the next
 * projection work.  This natural decomposition removes the former identical-arm
 * fence, pointer carriers, and the purported 15-byte allocation floor.
 */

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
extern long abs(long value);
extern int rand(void);
extern void *memset(void *dst, int value, u32 size);
extern void GsSortLine(GsLINE *line, GsOT *ot, u16 priority);

static inline void PrepareLightningScreenPosition(void)
{
    MATRIX *matrix = (MATRIX *)0x1F800000;

    matrix->t[0] = 0;
    matrix->t[1] = 0;
    matrix->t[2] = 0;
    SetTransMatrix(matrix);
    SetRotMatrix(&GsWSMATRIX);
}

static inline void GetLightningScreenPosition(long x, long y, long z,
                                              SVECTOR *screen)
{
    SVECTOR *vector = (SVECTOR *)0x1F800080;

    vector->vx = x - (short)ViewInfo.vpx;
    vector->vy = y - (short)ViewInfo.vpy;
    vector->vz = z - (short)ViewInfo.vpz;
    screen->vz = (s16)RotTransPers(vector, (s32 *)screen,
                                   (void *)0x1F800000,
                                   (void *)0x1F800010);
}

void SetLightningI(VECTOR *start, VECTOR *end, int gen, short r, short g, short b)
{
    SVECTOR scr;
    SVECTOR oldscr;
    GsLINE line;
    VECTOR sv;
    int next_gen;
    short lr;
    short lg;
    short lb;
    VECTOR *svp;
    long distance;
    long lcount;
    int i;
    int x;
    int y;
    int z;

    lr = r;
    next_gen = gen - 1;
    lg = g;
    lb = b;
    if (gen != 0)
    {
        PrepareLightningScreenPosition();
        GetLightningScreenPosition(start->vx, start->vy, start->vz, &oldscr);

        line.attribute = 0x50000000;
        line.r = lr;
        line.g = lg;
        line.b = lb;

        {
            VECTOR *v1, *v2;
            long dx, dy, dz;
            int large;

            v1 = start;
            v2 = end;
            large = 0;
            dx = v1->vx - v2->vx;
            dy = v1->vy - v2->vy;
            dz = v1->vz - v2->vz;
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
                    svp = &sv;
                    memset(svp, 0, sizeof(VECTOR));
                    sv.vx = x;
                    sv.vy = y;
                    sv.vz = z;
                    SetLightningI(svp, end, next_gen, lr, lg, lb);
                }

                GetLightningScreenPosition(x, y, z, &scr);

                if (((s32)(u16)scr.vz << 16) > 0 && oldscr.vz > 0)
                {
                    int z;
                    int p;

                    line.x0 = oldscr.vx;
                    line.y0 = oldscr.vy;
                    z = ((s32)(u16)scr.vz << 16) >> 18;
                    line.x1 = scr.vx;
                    line.y1 = scr.vy;
                    if (z >= 0)
                    {
                        if (z < 0x4e2)
                            p = z;
                        else
                            p = 0x4e1;
                    }
                    else
                    {
                        p = 0;
                    }
                    GsSortLine(&line, OTablePt, (u16)p);
                }
                oldscr = scr;
                i++;
            }
        }
    }
}

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
