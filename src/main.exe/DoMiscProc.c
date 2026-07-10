#include "common.h"
#include "main.exe.h"
#include "misc.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DoMiscProc(void);
 *     MISC.C:713, 19 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     reg   $s1       int i
 *     reg   $s1       int i
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TMisc misc[200];
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 44 of 452 bytes differ. Residual is a
 * register-allocation/cse difference in the distance-cull loop: the target
 * materializes `misc`'s base address directly into its walking-pointer
 * register (`addiu s0,v0,2392`) and shares ONE `%hi` computation between
 * `misc` and `ViewInfo` (their addresses are within 64K of each other, so
 * `addiu s2,v0,-32640` / `addiu s0,v0,2392` both read off the same `v0`);
 * our build instead computes `misc`'s address into a fresh register (s2)
 * and copies it into the walking pointer (`move s0,s2`) — one extra
 * instruction, and the loop's own exit test degrades from the target's
 * plain counter compare (`slti v0,s1,200`) to a pointer-vs-pointer bound
 * check (`addiu v0,s2,7200; slt v0,s0,v0`) since cc1 treats the array-index
 * base as still-referenced. Tried and reverted: a raw walking-pointer
 * `tag_TMisc *p` (matches ProcMiscDoor's convention) reintroduces the WORSE
 * "last-field-touched sits at offset 0" bias this array-indexed form was
 * written to avoid (misc[i].pause is written last in both branches,
 * biasing a raw pointer by +0x14 — confirmed via a direct trial, 117 vs 113
 * bytes); caching a local `tag_TMisc *p = &misc[i];` inside the null-check
 * fixes the loop-exit test back to a counter but adds its own extra
 * register (a `move s1,s0` preserving `p` across the call), netting 115 vs
 * 113. The array-indexed form (kept in this NON_MATCHING body) is the best
 * found: right total instruction COUNT (113 of 113), with the residual
 * confined to this first loop's preheader/exit-test register choices (the
 * second loop's own register NAMES differ too, but only as a renumbering
 * consequence of the first loop's extra register, not a separate issue).
 *
 * DoMiscProc (0x8004d350, 0x1C4 bytes) — the misc pool's per-frame driver
 * (main's game loop): bails with an error box if InitMisc hasn't run yet;
 * otherwise, every 10th GameClock tick, culls every live slot against a
 * 15000-unit cube around the camera (ViewInfo.vrx/vry/vrz) — dispatching
 * MM_RESUME(3)+unpause when back in range, MM_PAUSE(2)+pause when it drops
 * out — then unconditionally runs every still-unpaused slot's "draw" tick
 * (message 4) after setting the renderer's TMD mode.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `GameClock == (GameClock / 10) * 10` reproduces the div-by-10
 *    magic-multiply automatically (same idiom as DoItemProc's identical
 *    tick gate, same TU-independent shape).
 *  - The distance cull is Ghidra's own literal nesting incl. the
 *    `goto LAB_8004d49c` early-continue for the in-range case — transcribed
 *    as-is; the null-check-through-a-loaded-value + call-through-the-FIELD
 *    register convention (cookbook: guarded indirect call) matches without
 *    a named function-pointer local.
 *  - `DrawTMDmode = 0x20;` sits textually right after the cull loop in
 *    source, but its `li` is independent of the tick-gate branch, so cc1
 *    hoists it into that branch's own delay slot regardless of which side
 *    is taken — ordinary scheduling, no special spelling.
 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DoMiscProc", DoMiscProc);
#else
/* Draft — turn this into matching C, then delete the #ifndef/#else/#endif
   guards. Reference: */

typedef struct
{
    s32 vpx;
    s32 vpy;
    s32 vpz;
    s32 vrx;
    s32 vry;
    s32 vrz;
    s32 rz;
    void *super;
} GsRVIEW2;

extern s32 GameClock;
extern s32 DrawTMDmode;
extern GsRVIEW2 ViewInfo;

void DoMiscProc(void)
{
    s32 i;
    s32 d;

    if (EFFECT_SPAWNERS_INITIALISED == 0)
    {
        AdtMessageBox("misc not initialized");
    }
    else
    {
        if (GameClock == (GameClock / 10) * 10)
        {
            for (i = 0; i < 200; i++)
            {
                if (misc[i].proc != 0)
                {
                    d = ViewInfo.vrx - misc[i].x;
                    if (d < 0)
                        d = -d;
                    if (d < 15000)
                    {
                        d = ViewInfo.vry - misc[i].y;
                        if (d < 0)
                            d = -d;
                        if (d < 15000)
                        {
                            d = ViewInfo.vrz - misc[i].z;
                            if (d < 0)
                                d = -d;
                            if (d < 15000)
                            {
                                if (misc[i].pause != 0)
                                {
                                    misc[i].proc(&misc[i], 3);
                                    misc[i].pause = 0;
                                }
                                goto next;
                            }
                        }
                    }
                    if (misc[i].pause == 0)
                    {
                        misc[i].proc(&misc[i], 2);
                        misc[i].pause = 1;
                    }
                }
            next:;
            }
        }
        DrawTMDmode = 0x20;
        for (i = 0; i < 200; i++)
        {
            if (misc[i].proc != 0 && misc[i].pause == 0)
            {
                misc[i].proc(&misc[i], 4);
            }
        }
    }
}
#endif /* NON_MATCHING */

// triage: MEDIUM — 113 insns, mul/div, 2 loop, indirect-call, 1 callees, ~0.08 to ResetAllMisc
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void DoMiscProc(void)
//
// {
//   int iVar1;
//   tag_TMisc *ptVar2;
//   int iVar3;
//
//   if (DAT_80097c44 == '\0') {
//     AdtMessageBox("misc not initialized");
//   }
//   else {
//     if (GameClock == (GameClock / 10) * 10) {
//       iVar3 = 0;
//       ptVar2 = misc;
//       do {
//         if (ptVar2->proc != (undefined **)0x0) {
//           iVar1 = ViewInfo.vrx - ptVar2->x;
//           if (iVar1 < 0) {
//             iVar1 = -iVar1;
//           }
//           if (iVar1 < 15000) {
//             iVar1 = ViewInfo.vry - ptVar2->y;
//             if (iVar1 < 0) {
//               iVar1 = -iVar1;
//             }
//             if (iVar1 < 15000) {
//               iVar1 = ViewInfo.vrz - ptVar2->z;
//               if (iVar1 < 0) {
//                 iVar1 = -iVar1;
//               }
//               if (iVar1 < 15000) {
//                 if (ptVar2->pause != '\0') {
//                   (*(code *)ptVar2->proc)(ptVar2,3);
//                   ptVar2->pause = '\0';
//                 }
//                 goto LAB_8004d49c;
//               }
//             }
//           }
//           if (ptVar2->pause == '\0') {
//             (*(code *)ptVar2->proc)(ptVar2,2);
//             ptVar2->pause = '\x01';
//           }
//         }
// LAB_8004d49c:
//         iVar3 = iVar3 + 1;
//         ptVar2 = ptVar2 + 1;
//       } while (iVar3 < 200);
//     }
//     _DrawTMDmode = 0x20;
//     iVar3 = 0;
//     ptVar2 = misc;
//     do {
//       if ((ptVar2->proc != (undefined **)0x0) && (ptVar2->pause == '\0')) {
//         (*(code *)ptVar2->proc)(ptVar2,4);
//       }
//       iVar3 = iVar3 + 1;
//       ptVar2 = ptVar2 + 1;
//     } while (iVar3 < 200);
//   }
//   return;
// }
