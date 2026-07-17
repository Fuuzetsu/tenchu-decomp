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
 * STATUS: NON_MATCHING — 65 of 1384 linked bytes differ.  The candidate has
 * the target's 0x98-byte frame, 334 instructions, and exact control-flow
 * counts (24 conditional branches, 3 unconditional jumps, 16 calls, and one
 * return).
 *
 * This checkpoint cuts the previous 124-byte residual with pure-C allocation
 * and scheduling levers.  Single-use aliases recover the initial
 * RotTransPers argument identities, while erased identical arms and one-shot
 * loops weight the shared ViewInfo base and its projection values.  Grouping
 * the four line-coordinate stores around `raw_priority` in target source
 * order makes the complete depth/clamp/GsSortLine tail exact.  The remaining
 * differences are localized to the initial ViewInfo address/result register
 * cycle, placement of the 0x66666667 literal, and the final ViewInfo
 * scratchpad-store schedule; the function extent, frame, CFG, and calls remain
 * exact without volatile accesses, inline assembly, or output branches.
 *
 * 2026-07-17 evidence-complete pass (parked, no byte movement): reghist showed
 * delta sum 0 (v0 +4, t1 -2, t2 -2) with no OPCODES-DIFFER banner -- pure
 * allocation/scheduling, confirmed structure-correct.  tools/autorules.py (103
 * candidates) and tools/autorules.py --guided (160 candidates, rtlguide-keyed
 * to lines 165,169,190,191,233-235,259,260) both found zero improving edits.
 * A bounded permuter run (timeout 240 + 90s authoritative rescore) found
 * nothing better than base: best candidate output-1150-1 scored 77 (worse).
 * tools/rtlguide.py names the owner as `regalloc` (3 of 4 clusters) and prints
 * an explicit HARD-CONFLICT: pseudo p106 (`initial_view`) is INSEPARABLE-
 * packed into hard register $v1 alongside NINE unrelated pseudos
 * (p154/p159/p172/p180/p188/p196/p208/p220/p261 -- the `raw_priority`-family
 * values inside the hot loop, priority 26666-60000 from loop-depth-weighted
 * ref counts) per tools/regalloc.py --order's self-validated allocation
 * order.  gcc-2.8.1 has no coalescing pass, so any pseudo whose live range
 * does not CONFLICT with the top-priority in-loop claimant falls into $v1 for
 * free, regardless of where in the function it lives -- this ties the
 * cluster-0/1 register identity (initial_view, used once before the loop) to
 * unrelated code hundreds of instructions later, a genuine whole-function
 * register-pressure interaction rather than a local spelling choice.  Two
 * hypothesis-driven edits derived from this analysis were tested and reverted
 * as regressions: naming `vpy`/`vpz` temps at the loop-body ViewInfo access
 * let CSE unify the two independently-scheduled `high(ViewInfo)` computations
 * (target keeps them as two separate `lui`s into v0 and t6) and dropped one
 * instruction (65 -> LENGTH MISMATCH, 1380 vs 1384); rewriting the y/z
 * subtractions as in-place `initial_y -= ...` compound assignments (to match
 * target's `subu t1,t1,v0` self-reuse) reached the right SHAPE for that one
 * hunk but reassigned an unrelated load's register (`lw t2,8(s6)` -> `lw
 * t0,8(s6)`) and regressed net (65 -> 68).  Swapping the y/z statement order
 * regressed to 71; reordering `i = 1;`/`lcount = distance / 200;` was a
 * confirmed no-op (tools/nullcheck.py).  Cluster 2 (the 0x66666667 magic
 * constant vs. svp/scrp address setup, a pure MULTISET-EQUAL reorder) traces
 * to the same class of priority-driven scheduling choice via
 * tools/cc1says.py/sched-deps.py but has no tested fix.
 *
 * Sibling relationship: SetWire.c (similarity 0.30, also NON_MATCHING) writes
 * the analogous x/y/z-minus-ViewInfo scratchpad-store idiom in its OWN
 * clean/no-pointer-variable style (direct `ViewInfo.vpx/vpy/vpz`, no
 * `initial_view`-equivalent) and STILL carries the identical unresolved
 * residual class in its own header ("the entry VECTOR/ViewInfo loads and the
 * per-segment zero/coordinate stores... a bounded permuter run... produced
 * ONLY invalid candidates... treat the permuter as definitively unhelpful
 * here").  Two independent matchers, two independent source idioms for the
 * same original code shape, the same scheduler/regalloc residual class in
 * both: this is not idiom-specific.  Next lever, if picked back up: try
 * jointly perturbing the `raw_priority`/`priority` clamp code's own pseudo
 * identities (the actual $v1 claimant) together with the ViewInfo block,
 * since single-hunk edits on either side alone have now been exhausted on
 * both functions.
 */

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
    SVECTOR *initial_scrp;
    s32 *initial_screen;
    void *initial_matrix;
    void *initial_flag_base;
    u16 initial_vpx;
    LightningViewInfo *initial_view;

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
        initial_scrp = (SVECTOR *)0x1F800080;
        initial_screen = (s32 *)&oldscr;
        do
        {
            initial_matrix = (void *)0x1F800000;
            initial_flag_base = initial_matrix;
        } while (0);
        if (end != 0)
        {
            initial_view = &ViewInfo;
        }
        else
        {
            initial_view = &ViewInfo;
        }
        do
        {
            initial_vpx = (u16)initial_view->vpx;
        } while (0);
        {
            long initial_x;
            long initial_y;
            long initial_z;

            initial_x = start->vx;
            do
            {
                initial_y = start->vy;
            } while (0);
            if (end != 0)
            {
                initial_z = start->vz;
            }
            else
            {
                initial_z = start->vz;
            }
            *(s16 *)0x1F800080 = initial_x - initial_vpx;
            *(s16 *)0x1F800082 = initial_y - (s16)initial_view->vpy;
            *(s16 *)0x1F800084 = initial_z - (s16)initial_view->vpz;
        }
        oldscr.vz = (s16)RotTransPers(initial_scrp, initial_screen,
                                      initial_matrix, (char *)initial_flag_base + 0x10);

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
                        int raw_priority;
                        int priority;

                        do
                        {
                            line.x0 = oldscr.vx;
                            line.y0 = oldscr.vy;
                            raw_priority = depth >> 18;
                            line.x1 = scr.vx;
                            line.y1 = scr.vy;
                        } while (0);
                        if (raw_priority < 0)
                        {
                            goto priority_zero;
                        }
                        priority = 0x4e1;
                        if (raw_priority < 0x4e2)
                        {
                            priority = raw_priority;
                        }
                        goto priority_done;
priority_zero:
                        priority = 0;
priority_done:
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
