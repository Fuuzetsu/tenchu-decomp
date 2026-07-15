#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetSpline(struct SVECTOR *vect, struct SplineControlType *spc, short cnt);
 *     ACTION.C:388, 34 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s2       struct SVECTOR * vect
 *     param $s0       struct SplineControlType * spc
 *     param $s1       short cnt
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionPackType *StageMotion;
 *     extern struct NodeIndexType *FieldIndex;
 *     extern unsigned short *Command[12];
 * END PSX.SYM */

/*
 * GetSpline (0x8001c170, 0x150 bytes) — advance a bone's spline bracket
 * (spc->key0/key1) so it straddles `cnt`, re-deriving the per-frame deltas
 * (UpdateSplineControl) whenever the bracket actually moved, then evaluate
 * the spline at `cnt` via FUN_8001c730 (unmatched) using a memoized
 * fixed-point interpolation fraction (D_80097EEC, cached against D_80097708
 * so D_80097EE8's table-row address is only recomputed when the fraction
 * changes). PSX.SYM's globals (StageMotion/FieldIndex/Command) belong to the
 * demo build's differently-shaped callee — this retail body doesn't
 * reference them directly.
 *
 * Matching notes: the two do-while search loops are literal transcriptions
 * of Ghidra's own rendering (both walk `pMVar1`/`pMVar3` one MotionElementType
 * at a time, matching the asm's `addiu +-8` stride exactly — 8 bytes,
 * MotionElementType's proven size). The division is a plain `/`: this file
 * needs Build.hs's per-file --expand-div (ASPSX's guarded bnez/break 7/
 * break 6 sequence), same as GetAreaMapLevel.c/bow_shoot_logic.c. `iVar4==0`/
 * `iVar4==-1 && iVar2==MIN_INT` traps are --expand-div's own guard code, not
 * hand-written C. D_800868A4 is a large (non-`-G8`-small) table: its address
 * always fully materializes (`lui/addiu` into one register, EXPAND_SUM
 * expanding the `* 8` index scale first regardless of the `+` operand
 * order — cookbook Expressions).
 *
 * STATUS: NON_MATCHING — 238 of 336 bytes differ, but the instruction COUNT
 * is only 1 short (83 vs 84) and every diff traces to ONE root cause: the
 * `vect` parameter's `move s2,a0` (needed only by the final call, so it
 * survives across both calls in a callee-saved reg) sits as literally the
 * SECOND instruction in the target (right after the first register save),
 * but our compile schedules it into the first branch's delay slot instead —
 * `regalloc.py --rtl` confirms cc1's own -dg dump places the copy at the
 * same early RTL position (insn 4) in BOTH cases, so this is a POST-alloc
 * scheduling/reorg placement tie, not a structural or register-choice
 * difference. Leaving `a0` occupied by `vect` longer forces the `spc->key1`
 * pointer into a fresh register (`v1`) instead of reusing `a0` once it's
 * free, cascading through the rest of the function. Tried and confirmed
 * NO EFFECT: reading the compare through `pMVar1` vs re-reading `spc->key1`
 * literally; moving `pMVar1 = spc->key1;` inside the `if` (both directions —
 * either loses ANOTHER instruction, 82 vs 84); a `do{}while(0)` wrapper
 * around the whole dispatch+update-control block; reordering the local
 * declarations. A bounded decomp-permuter run (~350s, 29k+ iterations,
 * `--stop-on-zero -j4`) plateaued around score 690, never approaching 0 —
 * consistent with the cookbook's permuter-immune reload/scheduler-tie class
 * (see matching-cookbook.md's sub-C-level residual section). Root cause
 * documented; needs either a genuine source-shape insight or acceptance as
 * a compiler-version artifact.
 */
extern void UpdateSplineControl(SplineControlType *spc);
extern void FUN_8001c730(SVECTOR *vect, SplineControlType *spc, s32 row);
extern s16 D_80097708;
extern s16 D_80097EEC;
extern s32 D_80097EE8;
extern u8 D_800868A4[];

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GetSpline", GetSpline);
#else
void GetSpline(SVECTOR *vect, SplineControlType *spc, short cnt)
{
    MotionElementType *pMVar1;
    MotionElementType *pMVar3;
    s32 iVar2;
    s32 iVar4;

    pMVar1 = spc->key1;
    if (pMVar1->time < cnt) {
        do {
            pMVar3 = pMVar1;
            spc->key1 = pMVar3 + 1;
            pMVar1 = pMVar3 + 1;
        } while (pMVar3[1].time < cnt);
        spc->key0 = pMVar3;
    } else {
        pMVar1 = spc->key0;
        if (spc->key0->time <= cnt) {
            goto skip;
        }
        do {
            pMVar3 = pMVar1;
            spc->key0 = pMVar3 - 1;
            pMVar1 = pMVar3 - 1;
        } while (cnt < pMVar3[-1].time);
        spc->key1 = pMVar3;
    }
    UpdateSplineControl(spc);
skip:
    iVar4 = (s32)spc->key0->time;
    iVar2 = (cnt - iVar4) * 0x20;
    iVar4 = spc->key1->time - iVar4;
    D_80097EEC = (s16)(iVar2 / iVar4);
    if ((s32)D_80097708 != (s32)D_80097EEC) {
        D_80097EE8 = (s32)(D_800868A4 + D_80097EEC * 8);
        D_80097708 = D_80097EEC;
    }
    FUN_8001c730(vect, spc, D_80097EE8);
}
#endif
