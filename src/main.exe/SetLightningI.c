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
 * STATUS: NON_MATCHING — 15 of 1384 bytes differ. HUMAN-STRUCTURE REWRITE
 * (EFFECT.C:1500, same author/idiom as SetWire's adopted EFFECT.C:1428
 * rewrite), replacing the previous byte-chased 65 checkpoint per owner call.
 * Exact length, exact CFG, exact frame; the residual is 12 instructions of
 * pure same-position register renames in ONE cluster (the entry ViewInfo
 * block, 0x37938..0x37978).
 *
 * THE JOURNEY (each step measured): plain human transcription 118 ->
 * declaration order fixed 113 -> compound in-place entry subtractions
 * (x -= vpx; store x;) 89 -> loop vpz via pointer-assignment 53 ->
 * identical-arm fence on end 29 -> vpz-into-dead-x carrier 23 -> vpy via
 * pointer statement 15. Permuter round 7 (300s, 21.8k iters) retained
 * NOTHING better; autorules 76 candidates flat; rescoring each compensator
 * alone-removed at the end state: fence +32, vpzp +44, carrier +14,
 * vpyp +8(ish) — all four load-bearing, none redundant.
 *
 * WHAT THE HUMAN STRUCTURE PROVED (all byte-verified this round):
 *   - The in-loop svp/scrp assignment positions ARE the preheader order:
 *     loop.c emits hoists in loop-body SCAN order, so the original assigned
 *     &sv inside the if(rand&2) arm and scrp right before its use — that is
 *     the only way to get [0x66666667 magic][&sv+spill][&scr->fp], which the
 *     old draft's source-preheader assignments could never produce (hoists
 *     land AFTER source preheader insns).
 *   - Entry projection: the three lws batch (source temps, PSX.SYM's
 *     long x,y,z block) and ALL THREE subtractions are in-place (compound
 *     x -= ...), matching subu rX,rX,rY; the old draft's rvalue spelling
 *     needed fresh-temp subus, provably 8+ bytes away.
 *   - The old park's "compound = regression to 68/115" negatives were
 *     measured inside the fence-propped context and are VOID here.
 *   - lr/lg/lb narrow copies + next_gen at the top reproduce the prologue
 *     exactly (spill homes 0x48/0x50/0x58/0x60 = PSX.SYM's demo homes).
 *   - PSX.SYM's v1/v2 (distance block, s6/s7 = start/end's own regs) are
 *     copy-propagated by cse before flow counts refs: byte-neutral,
 *     kept as authentic text. GetVectorDistance's matched body transcribes
 *     otherwise verbatim (with dx/dy/dz per the reversed-listing rule).
 *
 * RECONSTRUCTION COMPENSATORS still in the text (marked, all four measured
 * load-bearing; the first two are almost certainly stand-ins for whatever
 * the original spelled differently):
 *   1. if (end->vy) identical-arm fence around zeros+matrix calls: its
 *      CONDITION donates weighted refs to `end`, lifting end (12 refs/460
 *      live = pri 782) past scrp (5/126 = 793) so end takes s7 and scrp fp
 *      as in the target; without it they swap (13 insns). The old 65 draft
 *      carried the same-family `if (end != 0)` fence — two independent
 *      searches converged on an end-reading discriminator here.
 *   2. vpzp pointer-assignment inside the loop's third scratchpad store:
 *      the REG_EQUIV-const pointer set survives until reload (then deleted,
 *      zero final-length cost) and re-routes the loop cluster's local-alloc
 *      qty map; fixes the whole loop ViewInfo cluster (36 bytes there).
 *   3. x = (u16)ViewInfo.vpz; z -= x; — the entry vpz read hosted in the
 *      dead x (multi-set exiles x's pseudo from local-alloc to global).
 *   4. vpyp = &ViewInfo.vpy; y -= (u16)*vpyp; — same EQUIV-set family as 2.
 *
 * THE RESIDUAL (evidence-complete this round): one local-alloc qty->reg
 * walk rotation over {hi, vpx, x, z, vpy, vpz} in the entry block:
 * ours {v0, v1, t2, t0, v1, t2} vs target {v1, t0, v0, t2, v0, v1}
 * (y=t1 matches). .lreg gives the real per-qty stats (x 4/12, y 4/12,
 * z 4/10, hi 3/6, base 3/10, vpx/vpy/vpz 2/3) but the OBSERVED dispositions
 * are inconsistent with every ordering derivable from QTY_CMP_PRI
 * (descending, ascending, pseudo-number, shortest-first) — the walk order
 * needs local-alloc.c's block_alloc actually traced. TOOL TICKET
 * (regalloc --local): print local-alloc's qty walk (order, birth/death
 * luids, suggestion passes) the way --order does for global allocnos; this
 * residual class is invisible to every current tool (still absent as of
 * this round — regalloc.py --help carries no --local flag).
 *
 * ROUND 2 (this round — restarted from 29 per the handoff, as directed):
 *   - Fresh bounded permuter (post -fno-builtin fix, so the pre-existing
 *     negatives were void per the day's brief) found NOTHING better from
 *     EITHER seed: 240s/~17.9k iters from the banked 15 (best stayed
 *     base.c); a second 240s/~18k-iter run from the 21-byte checkpoint
 *     below (a structurally different, pure-operand-diff neighbourhood the
 *     15-seeded search never saw) also stayed at base. Both authoritative
 *     rescores confirm: no candidate beat its own seed.
 *   - Manually traced local-alloc.c's block_alloc/qty_compare/QTY_CMP_PRI
 *     against the raw .lreg + .sched dumps (ccsrc.py) and derived the
 *     mechanism the earlier TOOL TICKET couldn't reach by formula alone:
 *     the target's register reuse is TWO CHAINS — {hi,base,vpz} on v1 (the
 *     address computation naturally extends into holding vpz's own value,
 *     since base is only consumed once more after vpz's load) and {x,vpy}
 *     on v0 (needs the explicit carrier, since x's death and vpy's birth
 *     are otherwise unrelated). The chain only self-overwrites correctly
 *     when vpy's access is scheduled BEFORE vpz's — if vpz's load happens
 *     first, base is still needed for the upcoming vpy load and can't be
 *     overwritten, forcing an extra register. Every combination that let
 *     vpz's access get prioritized ahead of vpy's (below) cost bytes.
 *   - Exhaustively swept the reachable Y/Z carrier x pointer combinations
 *     on the 29-byte base (each independently built and measured):
 *       plain Y      + plain Z      = 29 (the base)
 *       pointer Y    + plain Z      = 29 (nullcheck: real edit, 0 effect —
 *                                     a REG_EQUIV pointer alone does nothing
 *                                     without the carrier already present)
 *       carrier(x) Y + plain Z      = 28
 *       carrier(x) Y + carrier(y) Z = LENGTH MISMATCH (grew 4 bytes)
 *       carrier(x) Y + carrier(x) Z = 77, 10 clusters (x's pseudo reused a
 *                                     3rd time destabilizes GLOBAL alloc
 *                                     function-wide — the compiler-facts.md
 *                                     "+1 cascade" class, not a local tie)
 *       carrier(x) Y + pointer Z    = 21 (closest non-banked point; still
 *                                     wrong because Z's fresh pointer beats
 *                                     Y's carrier for scheduling priority,
 *                                     violating the vpy-before-vpz order)
 *       pointer Y    + pointer Z    = 29
 *       pointer(vpyp)Y + carrier(x) Z = 15 (the banked form — confirmed the
 *                                     unique best of this space)
 *     15 is a genuine local optimum of the reachable carrier/pointer
 *     combinator space, not an overlooked spot in it.
 *   - Also tested reversing the entry `long x, y, z;` block's declaration
 *     order to `long z, y, x;` per PSX.SYM's reverse-listing rule (listed
 *     x,y,z -> declared z,y,x): nullcheck confirms a real codegen change,
 *     but the final bytes are IDENTICAL to the unreversed form — a new
 *     verified-inert data point, not a lever here (sched1's block-scan
 *     birth order evidently doesn't chain back to raw declaration REGNO in
 *     a way that survives to the qty walk for this specific block).
 *   - Net: no ordering, pointer/carrier combination, or permuter seed
 *     reachable this round beats 15. The remaining gap is specifically
 *     WHY the target's sched1 places vpy's load before x's store even
 *     dies (an anti-dependency the carrier reproduces) while ALSO ordering
 *     vpy before vpz with no comparable source-level anti-dependency
 *     forcing that second half — i.e. two coupled scheduling decisions,
 *     and only one is currently reproducible from C. The no-carrier 29
 *     form's note (vpz recycles the base register the way the target does)
 *     still stands as the structurally-closer starting shape; what's now
 *     also established is that the SPECIFIC combination needed on top of
 *     it is narrower than "the walk order" alone suggests — it is this
 *     Y-before-Z scheduling coupling, not just a qty priority reordering.
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
} GsLINE;

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
    SVECTOR *scrp;
    s32 *vpyp;
    s32 *vpzp;
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
        /* COMPENSATOR 1 (see header): identical arms; the end-reading
         * condition weights `end` past scrp in global-alloc (end->s7,
         * scrp->fp). Cross-jump erases the branch; semantics unchanged. */
        if (end->vy)
        {
            *(s32 *)0x1F800014 = 0;
            *(s32 *)0x1F800018 = 0;
            *(s32 *)0x1F80001C = 0;
            SetTransMatrix((MATRIX *)0x1F800000);
            SetRotMatrix(&GsWSMATRIX);
        }
        else
        {
            *(s32 *)0x1F800014 = 0;
            *(s32 *)0x1F800018 = 0;
            *(s32 *)0x1F80001C = 0;
            SetTransMatrix((MATRIX *)0x1F800000);
            SetRotMatrix(&GsWSMATRIX);
        }
        {
            long x, y, z;

            x = start->vx;
            y = start->vy;
            z = start->vz;
            x -= (u16)ViewInfo.vpx;
            *(s16 *)0x1F800080 = x;
            /* COMPENSATORS 3+4 (see header): vpy through the pointer
             * statement, vpz hosted in the dead x — pure local-alloc
             * identity steering, values unchanged. */
            vpyp = &ViewInfo.vpy;
            y -= (u16)*vpyp;
            *(s16 *)0x1F800082 = y;
            x = (u16)ViewInfo.vpz;
            z -= x;
            *(s16 *)0x1F800084 = z;
        }
        oldscr.vz = (s16)RotTransPers((SVECTOR *)0x1F800080, (s32 *)&oldscr,
                                      (void *)0x1F800000, (void *)0x1F800010);

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

                *(s16 *)0x1F800080 = x - (u16)ViewInfo.vpx;
                *(s16 *)0x1F800082 = y - (u16)ViewInfo.vpy;
                /* COMPENSATOR 2 (see header): the vpzp EQUIV-const set
                 * lives until reload, re-routing this cluster's qty map;
                 * deleted there — zero final-length cost. */
                *(s16 *)0x1F800084 = z - (u16)*(vpzp = &ViewInfo.vpz);
                scrp = &scr;
                scrp->vz = (s16)RotTransPers((SVECTOR *)0x1F800080, (s32 *)scrp,
                                             (void *)0x1F800000, (void *)0x1F800010);

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
