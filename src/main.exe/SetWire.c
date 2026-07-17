#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetWire(struct VECTOR *start, struct VECTOR *end, struct VECTOR *center, long len);
 *     EFFECT.C:1428, 68 src lines, frame 120 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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
 *     param $s3       struct VECTOR * start
 *     param $s2       struct VECTOR * end
 *     param $s1       struct VECTOR * center
 *     param $s0       long len
 *     stack sp+16     struct VECTOR StockCenter
 *     reg   $s4       long lcount
 *     reg   $s0       int i
 *     reg   $s5       int ecount
 *     stack sp+32     struct SVECTOR scr
 *     stack sp+40     struct SVECTOR oldscr
 *     stack sp+72     int x
 *     stack sp+76     int y
 *     reg   $s6       int z
 *     stack sp+48     struct GsLINE line
 *     reg   $v0       long x
 *     reg   $a1       long y
 *     reg   $a2       long z
 *     reg   $s3       struct VECTOR * v1
 *     reg   $s2       struct VECTOR * v2
 *     reg   $a1       long dz
 *     reg   $a3       long dy
 *     reg   $t0       long dx
 *     reg   $v0       long t
 *     reg   $a0       long Q
 *     reg   $a2       long R
 *     stack sp+72     long x
 *     stack sp+76     long y
 *     reg   $s6       long z
 *     reg   $v1       int z
 *     stack sp+64     int rx
 *     stack sp+68     int ry
 *     reg   $s3       struct VECTOR * start
 *     reg   $s2       struct VECTOR * end
 *     reg   $s0       int dz
 *     reg   $s2       int dy
 *     reg   $s1       int dx
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 *     extern struct GsOT *OTablePt;
 *     extern struct ModelType *ModelHook;
 * END PSX.SYM */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetWire", SetWire);
#else
/*
 * STATUS: NON_MATCHING — 80 of 1488 bytes differ. HUMAN-STRUCTURE REWRITE
 * (the EFFECT.C:1428 experiment): written from PSX.SYM's own declarations and
 * blocks, no fences, no seed temps, no goto webs. The fully natural spelling
 * measures 85; the ONE compensator kept below (the loop cluster's zero/store
 * statement order, marked "COMPENSATOR") buys 85 -> 80. The previous 70 was
 * fence-propped (its own park refuted the fence by sched2 evidence) and is
 * banked in git history — do not resurrect it; this is the honest base.
 *
 * WHAT THE HUMAN STRUCTURE PROVED (all byte-verified this round):
 *   - The ENTIRE t/Q/R interpolation island matches from the author's own
 *     spelling: single-set `one = 0x1000`, `t = one - i*0x1000/lcount`,
 *     `Q = t*2`, `R = t*t/0x1000`, `A = one - Q + R`, `B = Q - R*2`, and
 *     x/y/z = (A*end + B*center + R*start)/0x1000 — where the old draft
 *     needed 8 temps and dead-carrier reuse. PSX.SYM's homes (t=$v0, Q=$a0,
 *     R=$a2) equal retail's for t, t*2, t*t/4096.
 *   - The priority clamp matches as a plain nested if/else with the PSX.SYM
 *     `int z` shadow (old draft: goto web). `p = z<0x4e2 ? z : 0x4e1` shape
 *     comes out of reorg's delay-slot fill on its own.
 *   - The distance block is GetVectorDistance's matched body (EFFECT.C:509)
 *     inlined; the tail is GetVectorRotation's (EFFECT.C:532) on
 *     (start, end, &rx, &ry) with pointer locals (see rxp/ryp note below).
 *   - `i * 0x1000` == `(i << 12)`; `long abs(long)`; (u16)/(s16) casts on
 *     ViewInfo reads are byte-neutral here.
 *
 * THE RESIDUAL (two clusters, both pure emission order, all priority-1
 * sched2 ties):
 *   1. Entry ViewInfo cluster (~50B, 0x2f0..0x368): retail needs allocation
 *      {x:v0, hi:v1, y:a1, z:a2, vpx:a3, vpy:v0-recycled, vpz:v1-recycled} —
 *      then every position follows from anti-deps (vpx->a3 is pinned UNDER
 *      `sw a3,0x84` = len's home save; vpy->v0 under x's death; vpz->v1
 *      under the addiu base). Ours allocates {hi:a2, vpx:v1, vpy:a1, vpz:a2}
 *      because THIS cc1's sched1 statement-sinks the single-use y/z loads
 *      below the x-store before RA runs, so vpy/vpz are born early and take
 *      a1/a2. Measured levers, all: statement order is INVISIBLE (zeros
 *      first/mid/last, x-store-first: all 85/80); subtract-first = 111;
 *      compound `-=` = 115; entry store scramble = 87; (u16)=(s16);
 *      autorules 92 candidates = 0; bounded permuter (240s+90s, post
 *      -fno-builtin) best VALID = base (its 82 "win" was `y & 0xFFu` — an
 *      lbu, semantically wrong, rejected).
 *   2. Loop store cluster (~30B, 0x6c4..0x6fc): same class; the statement-
 *      order compensator below recovers 5 of it (a 14-combo sweep of
 *      zero/store orders found zeros 18,1C,14 + stores x,z,y optimal — the
 *      same tuning the old draft carried; natural zeros + x,z,y = 82,
 *      scrambled zeros + x,y,z = 83; y-first/z-first orders shift length).
 *
 * DEMO-BUILD FACTS THIS ROUND ESTABLISHED (tools/siblingdiff.py --demo):
 *   - Demo and retail agree byte-for-byte-in-structure on BOTH residual
 *     clusters (incl. lui ViewInfo among the prologue saves and the late
 *     `sra z`) — the original source hits them from a plain spelling that
 *     this draft cannot reach; the divergence is in sched1's sink choices.
 *   - The demo loop is ROTATED (guard blez + bottom test) with `li 4096`
 *     and `&ViewInfo` HOISTED to the preheader; retail is top-test with the
 *     li in-loop. With cookbook §3.14 (user-var invariant + maybe_never)
 *     this pins the original as having a SINGLE-SET `one`-style variable,
 *     and means the author CHANGED the loop between builds (demo: plain
 *     for/while that rotated; retail: break-style that cannot).
 *   - Demo abs() was builtin/macro (inline branch+negate, NO flag); retail
 *     calls abs() and materializes the flag in $s5 — the author edited this
 *     between builds; GetVectorDistance's retail body is the proven shape.
 *
 * REUSABLE FACTS (report to orchestrator for the cookbook):
 *   - PSX.SYM lists block locals in REVERSE declaration order: writing
 *     `long dz, dy, dx;` as listed swapped s0/s1 vs target; the declared
 *     order was dx, dy, dz (pseudos are created at DECLARATION for block
 *     scalars, so multi-declarator order is codegen-relevant).
 *   - `*(&x) = v` does NOT stack-force x in this cc1: the fold beats
 *     mark_addressable (frame shrank 8 bytes, sw/lhu roundtrip vanished).
 *     A macro out-param needs a real pointer local (`int *rxp = &rx` /
 *     draft's rxp/ryp) to keep the address taken.
 *   - Rvalue pow2 division on a fresh temp (`t*t/0x1000`, sums, lcount*len)
 *     emits the IN-PLACE `bgez; addiu 0xFFF; sra` shape; only a store-back
 *     div on a user var (`dx /= 0x100`) emits the copy-in-delay shape — so
 *     the human's plain `/ONE` spellings reproduce both shapes with no
 *     explicit rounding code.
 */

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
} WireViewInfo;

typedef struct
{
    GsCOORDINATE2 locate;
    SVECTOR rotate;
} WireModel;

extern WireViewInfo ViewInfo;
extern MATRIX GsWSMATRIX;
extern GsOT *OTablePt;
extern WireModel *ModelHook;

extern void SetTransMatrix(MATRIX *matrix);
extern void SetRotMatrix(MATRIX *matrix);
extern s32 RotTransPers(SVECTOR *vector, s32 *screen, void *p, void *flag);
extern s32 SquareRoot0(s32 value);
extern s32 ratan2(s32 y, s32 x);
extern long abs(long value);
extern void GsSortLine(GsLINE *line, GsOT *ot, u16 priority);
extern void UpdateCoordinate(WireModel *model);
extern void DrawModel(WireModel *model);

void SetWire(VECTOR *start, VECTOR *end, VECTOR *center, long len)
{
    VECTOR StockCenter;
    long lcount;
    int i;
    int ecount;
    SVECTOR scr;
    SVECTOR oldscr;
    int x, y, z;
    GsLINE line;
    long distance;

    {
        long x, y, z;

        x = start->vx;
        y = start->vy;
        z = start->vz;
        *(s32 *)0x1F800014 = 0;
        *(s32 *)0x1F800018 = 0;
        *(s32 *)0x1F80001C = 0;
        *(s16 *)0x1F800020 = x - (u16)ViewInfo.vpx;
        *(s16 *)0x1F800022 = y - (u16)ViewInfo.vpy;
        *(s16 *)0x1F800024 = z - (u16)ViewInfo.vpz;
    }
    SetTransMatrix((MATRIX *)0x1F800000);
    SetRotMatrix(&GsWSMATRIX);
    oldscr.vz = (s16)RotTransPers((SVECTOR *)0x1F800020, (s32 *)&oldscr,
                                  (void *)0x1F800028, (void *)0x1F80002C);

    line.attribute = 0;
    line.r = 0x50;
    line.g = 0x48;
    line.b = 0x38;

    {
        long dx, dy, dz;
        int big;

        dx = start->vx - end->vx;
        dy = start->vy - end->vy;
        dz = start->vz - end->vz;
        big = 0;
        if (abs(dx) > 0x1000 || abs(dy) > 0x1000 || abs(dz) > 0x1000)
        {
            big = 1;
        }
        if (big)
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

    lcount = distance / 300;
    if (center == 0)
    {
        StockCenter.vx = (end->vx + start->vx) / 2;
        center = &StockCenter;
        center->vy = (end->vy + start->vy) / 2 + distance / 32;
        center->vz = (end->vz + start->vz) / 2;
    }

    ecount = lcount * len / 0x1000;
    i = 0;
    while (1)
    {
        long t, Q, R;
        long one, A, B;

        if (i >= ecount)
        {
            break;
        }

        one = 0x1000;
        t = one - i * 0x1000 / lcount;
        Q = t * 2;
        R = t * t / 0x1000;
        A = one - Q + R;
        B = Q - R * 2;
        x = (A * end->vx + B * center->vx + R * start->vx) / 0x1000;
        y = (A * end->vy + B * center->vy + R * start->vy) / 0x1000;
        z = (A * end->vz + B * center->vz + R * start->vz) / 0x1000;

        /* COMPENSATOR: statement order only. The human order (zeros
         * 14,18,1C + stores x,y,z) measures +5; this order makes sched2's
         * tie-breaks emit the target's schedule. Retail's own emitted order
         * IS the natural one — this scramble stands in for an upstream
         * sched1/allocation difference, not for the original text. */
        *(s32 *)0x1F800018 = 0;
        *(s32 *)0x1F80001C = 0;
        *(s32 *)0x1F800014 = 0;
        *(s16 *)0x1F800020 = x - (u16)ViewInfo.vpx;
        *(s16 *)0x1F800024 = z - (u16)ViewInfo.vpz;
        *(s16 *)0x1F800022 = y - (u16)ViewInfo.vpy;
        SetTransMatrix((MATRIX *)0x1F800000);
        SetRotMatrix(&GsWSMATRIX);
        scr.vz = (s16)RotTransPers((SVECTOR *)0x1F800020, (s32 *)&scr,
                                   (void *)0x1F800028, (void *)0x1F80002C);

        if (((s32)scr.vz << 16) > 0 && oldscr.vz > 0)
        {
            int z;
            int p;

            line.x0 = oldscr.vx;
            line.y0 = oldscr.vy;
            z = ((s32)scr.vz << 16) >> 18;
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

    {
        int rx, ry;

        {
            int *rxp, *ryp;
            int dx, dy, dz;

            dx = end->vx - start->vx;
            dz = end->vz - start->vz;
            dy = end->vy - start->vy;
            rxp = &rx;
            ryp = &ry;
            *ryp = ratan2(-dx, -dz);
            *rxp = ratan2(dy, SquareRoot0(dx * dx + dz * dz));
        }
        ModelHook->locate.coord.t[0] = x;
        ModelHook->locate.coord.t[1] = y;
        ModelHook->locate.coord.t[2] = z;
        ModelHook->rotate.vx = rx;
        ModelHook->rotate.vy = ry;
        ModelHook->rotate.vz = 0;
        UpdateCoordinate(ModelHook);
        DrawModel(ModelHook);
    }
}

#endif

// triage: HARD — 372 insns, mul/div, 9 callees, ~0.10 to GetVectorRotation
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SetWire(VECTOR *start,VECTOR *end,VECTOR *center,long len)
//
// {
//   bool bVar1;
//   ModelType *dim;
//   long lVar2;
//   int iVar3;
//   int iVar4;
//   int iVar5;
//   int iVar6;
//   int iVar7;
//   int iVar8;
//   int iVar9;
//   int iVar10;
//   long unaff_s7;
//   long unaff_s8;
//   VECTOR local_68;
//   undefined4 local_58;
//   undefined4 local_54;
//   undefined4 local_50;
//   undefined4 local_4c;
//   GsLINE local_48;
//   long local_38;
//   long local_34;
//   long local_30;
//
//   Scratchpad[0x14] = 0;
//   Scratchpad[0x15] = 0;
//   Scratchpad[0x16] = 0;
//   Scratchpad[0x17] = 0;
//   Scratchpad[0x18] = 0;
//   Scratchpad[0x19] = 0;
//   Scratchpad[0x1a] = 0;
//   Scratchpad[0x1b] = 0;
//   Scratchpad[0x1c] = 0;
//   Scratchpad[0x1d] = 0;
//   Scratchpad[0x1e] = 0;
//   Scratchpad[0x1f] = 0;
//   Scratchpad._32_2_ = (short)start->vx - (short)ViewInfo.vpx;
//   Scratchpad._34_2_ = (short)start->vy - (short)ViewInfo.vpy;
//   Scratchpad._36_2_ = (short)start->vz - (short)ViewInfo.vpz;
//   SetTransMatrix((MATRIX *)Scratchpad);
//   SetRotMatrix(&GsWSMATRIX);
//   lVar2 = RotTransPers((SVECTOR *)(Scratchpad + 0x20),&local_50,(long *)(Scratchpad + 0x28),
//                        (long *)(Scratchpad + 0x2c));
//   local_4c = CONCAT22(local_4c._2_2_,(short)lVar2);
//   local_48.r = 'P';
//   local_48.g = 'H';
//   bVar1 = false;
//   local_48.attribute = 0;
//   local_48.b = '8';
//   iVar10 = start->vx - end->vx;
//   iVar8 = start->vy - end->vy;
//   iVar9 = start->vz - end->vz;
//   iVar3 = abs(iVar10);
//   if (((0x1000 < iVar3) || (iVar3 = abs(iVar8), 0x1000 < iVar3)) ||
//      (iVar3 = abs(iVar9), 0x1000 < iVar3)) {
//     bVar1 = true;
//   }
//   if (bVar1) {
//     if (iVar10 < 0) {
//       iVar10 = iVar10 + 0xff;
//     }
//     if (iVar8 < 0) {
//       iVar8 = iVar8 + 0xff;
//     }
//     if (iVar9 < 0) {
//       iVar9 = iVar9 + 0xff;
//     }
//     lVar2 = SquareRoot0((iVar10 >> 8) * (iVar10 >> 8) + (iVar8 >> 8) * (iVar8 >> 8) +
//                         (iVar9 >> 8) * (iVar9 >> 8));
//     lVar2 = lVar2 << 8;
//   }
//   else {
//     lVar2 = SquareRoot0(iVar10 * iVar10 + iVar8 * iVar8 + iVar9 * iVar9);
//   }
//   iVar3 = lVar2 / 300;
//   if (center == (VECTOR *)0x0) {
//     local_68.vx = (end->vx + start->vx) / 2;
//     center = &local_68;
//     if (lVar2 < 0) {
//       lVar2 = lVar2 + 0x1f;
//     }
//     local_68.vy = (end->vy + start->vy) / 2 + (lVar2 >> 5);
//     local_68.vz = (end->vz + start->vz) / 2;
//   }
//   iVar10 = iVar3 * len;
//   if (iVar10 < 0) {
//     iVar10 = iVar10 + 0xfff;
//   }
//   for (iVar8 = 0; iVar8 < iVar10 >> 0xc; iVar8 = iVar8 + 1) {
//     if (iVar3 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar3 == -1) && (iVar8 << 0xc == -0x80000000)) {
//       trap(0x1800);
//     }
//     iVar4 = 0x1000 - (iVar8 << 0xc) / iVar3;
//     iVar9 = iVar4 * iVar4;
//     if (iVar9 < 0) {
//       iVar9 = iVar9 + 0xfff;
//     }
//     iVar9 = iVar9 >> 0xc;
//     iVar7 = iVar4 * -2 + 0x1000 + iVar9;
//     iVar6 = iVar4 * 2 + iVar9 * -2;
//     iVar4 = iVar7 * end->vx + iVar6 * center->vx + iVar9 * start->vx;
//     if (iVar4 < 0) {
//       iVar4 = iVar4 + 0xfff;
//     }
//     iVar5 = iVar7 * end->vy + iVar6 * center->vy + iVar9 * start->vy;
//     local_30 = iVar4 >> 0xc;
//     if (iVar5 < 0) {
//       iVar5 = iVar5 + 0xfff;
//     }
//     iVar9 = iVar7 * end->vz + iVar6 * center->vz + iVar9 * start->vz;
//     unaff_s8 = iVar5 >> 0xc;
//     if (iVar9 < 0) {
//       iVar9 = iVar9 + 0xfff;
//     }
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
//     Scratchpad._32_2_ = (short)(iVar4 >> 0xc) - (short)ViewInfo.vpx;
//     Scratchpad._34_2_ = (short)unaff_s8 - (short)ViewInfo.vpy;
//     unaff_s7 = iVar9 >> 0xc;
//     Scratchpad._36_2_ = (short)unaff_s7 - (short)ViewInfo.vpz;
//     SetTransMatrix((MATRIX *)Scratchpad);
//     SetRotMatrix(&GsWSMATRIX);
//     lVar2 = RotTransPers((SVECTOR *)(Scratchpad + 0x20),&local_58,(long *)(Scratchpad + 0x28),
//                          (long *)(Scratchpad + 0x2c));
//     local_54 = CONCAT22(local_54._2_2_,(short)lVar2);
//     if ((0 < lVar2 << 0x10) && (0 < (short)local_4c)) {
//       local_48.y0 = local_50._2_2_;
//       iVar9 = (lVar2 << 0x10) >> 0x12;
//       local_48.x0 = (short)local_50;
//       local_48.x1 = (short)local_58;
//       local_48.y1 = local_58._2_2_;
//       if (iVar9 < 0) {
//         iVar4 = 0;
//       }
//       else {
//         iVar4 = 0x4e1;
//         if (iVar9 < 0x4e2) {
//           iVar4 = iVar9;
//         }
//       }
//       GsSortLine(&local_48,OTablePt,(ushort)iVar4);
//     }
//     local_50 = local_58;
//     local_4c = local_54;
//   }
//   iVar9 = end->vy;
//   iVar8 = end->vx - start->vx;
//   iVar10 = end->vz - start->vz;
//   iVar3 = start->vy;
//   local_34 = ratan2(-iVar8,-iVar10);
//   lVar2 = SquareRoot0(iVar8 * iVar8 + iVar10 * iVar10);
//   local_38 = ratan2(iVar9 - iVar3,lVar2);
//   dim = DAT_80097f28;
//   (DAT_80097f28->locate).coord.t[0] = local_30;
//   (dim->locate).coord.t[1] = unaff_s8;
//   (dim->locate).coord.t[2] = unaff_s7;
//   (dim->rotate).vx = (short)local_38;
//   (dim->rotate).vy = (short)local_34;
//   (dim->rotate).vz = 0;
//   UpdateCoordinate(dim);
//   DrawModel(DAT_80097f28);
//   return;
// }
