#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct TraceLine * SetupTraceLine(struct Humanoid *human, struct TracePoint *point);
 *     HUMAN.C:488, 18 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct Humanoid * human
 *     param $a1       struct TracePoint * point
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetupTraceLine", SetupTraceLine);

// triage: MEDIUM — 53 insns, mul/div, 1 loop, 2 callees, ~0.13 to AfsRead
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// TraceLine * SetupTraceLine(Humanoid *human,TracePoint *point)
//
// {
//   short sVar1;
//   TraceLine *pTVar2;
//
//   if (point == (TracePoint *)0x0) {
//                     /* WARNING: Subroutine does not return */
//     SystemOut((uchar *)"NO TRACE POINT");
//   }
//   pTVar2 = (TraceLine *)valloc(8);
//   pTVar2->count = 0;
//   pTVar2->index = 0;
//   pTVar2->point = point;
//   sVar1 = point->pad;
//   while (sVar1 != -1) {
//     sVar1 = point[1].pad;
//     point = point + 1;
//   }
//   point->x = human->locate->vx;
//   point->z = human->locate->vz;
//   point->range = (short)(human->locate->vy / 100);
//   human->trace = pTVar2;
//   return pTVar2;
// }
