#include "common.h"
#include "main.exe.h"
#include "gte.h"

/*
 * FUN_80059008 (0x80059008, 0x398 bytes) — a DrawTMD-family fast primitive
 * renderer (TMD primitive code 0x2d), the sibling of the switch arms in
 * FUN_80058a54: it transforms each quad's four vertices through the GTE
 * (RTPT + a backface NCLIP + a per-quad RTPS) and assembles a POLY_GT4
 * (code 0x3c, len 0xc) packet, then hands the working state to FUN_80057b80.
 * Compiled-style GTE function under the restricted gte.h policy
 * (docs/gte-policy.md); the COP2 moves and GTE commands are the original
 * INLINE_N.H mechanism.
 *
 * This function is INSTRUCTION-FOR-INSTRUCTION IDENTICAL to matched-adjacent
 * FUN_80058c70 (same 0x398 size, same registers, same call-adjacent stack
 * spills, same everything) except for three numeric differences, confirmed
 * by disassembling both targets directly (`tools/tdis.py`):
 *   1. Per-vertex cursor stride: `puVar4 += 0x10` (0x20 = 32 bytes), not
 *      FUN_80058c70's `+0x16` (0x2C = 44 bytes) — matches the caller
 *      FUN_80058a54's own `iVar1 = *puVar4 << 5` for case 0x2d, vs its
 *      `*puVar4 * 0xb; iVar1 <<= 2` (also 44) for case 0x3d.
 *   2. Vertex-index halfword offsets are puVar4[7,8,9,10] (bytes 14,16,18,20),
 *      not FUN_80058c70's puVar4[0xd,0xe,0xf,0x10] (bytes 26,28,30,32).
 *   3. The four `param_7[0x22/0x28/0x2e/0x34]` word fields all read the SAME
 *      source word `*(u_long *)(puVar4 + 5)` (target: `lw v0,10(s1)` FOUR
 *      TIMES, confirmed via `tools/access.py`: "$s1 +0x00a s32 load [lw] x4"
 *      — a single offset, four uses). FUN_80058c70 reads four DIFFERENT
 *      offsets there (puVar4+5,+7,+9,+0xb). This is a genuine source/data
 *      difference (flat per-face attribute vs. FUN_80058c70's per-vertex
 *      one), not a Ghidra artifact — Ghidra's own raw decompile of THIS
 *      function already showed the repeated `puVar4[5]->unk...` shape before
 *      any cleanup.
 * Every other offset (-3,-1,1,3,-2,0 relative to the cursor; all of param_7's
 * header/loop-body field offsets; the call-site register roles) is IDENTICAL
 * between the two targets, byte for byte — including the parts of
 * FUN_80058c70 still NON_MATCHING.
 *
 * STATUS: NON_MATCHING — 620 of 920 byte values differ (correct length,
 * 230/230 instructions, `tools/matchdiff.py` — no LENGTH MISMATCH). This
 * draft inherits FUN_80058c70.c's full autopsy wholesale (goto-loop vs
 * do-while, int vs u16 counter, the uVar3/iVar3 split, the caller-pinned
 * prototype from FUN_80058a54) and it lands on the EXACT SAME residual size:
 * 620/920, matching FUN_80058c70's own park to the byte. That is not a
 * coincidence — it is independent cross-validation of the same compiler
 * limitation on a second function, confirmed several ways:
 *
 *  - `tools/matchdiff.py --clusters`: 2 clusters, 218/230 differing insns —
 *    NOT a localized tie. Cluster 1 alone (0x800590a0..0x8005a3a0, 185
 *    insns/511 bytes) covers almost the whole function body including the
 *    loop, i.e. one root cause cascades into a whole-function register
 *    renumbering, exactly as FUN_80058c70's header describes.
 *  - `tools/cc1says.py` `.greg`: pseudo 80 (param_1) conflicts with hard reg
 *    4 ($a0) — the SAME conflict FUN_80058c70's header cites verbatim
 *    ("param_1 (reg 80) conflicts with $a0"). Root cause: cc1 evacuates
 *    param_1 out of $a0 immediately (`move t0,a0` is the 2nd emitted insn)
 *    so it can load param_6 into the now-free $a0 right away; the target
 *    instead keeps param_1 resident in $a0 for the whole prologue and only
 *    loads param_6 late, into $v0, after it frees up post-division
 *    (confirmed directly off this function's own disassembly: target
 *    `lw v0,92(sp)` comes after both `sra` divisions; ours loads param_6
 *    into $a0/$a1 as the 2nd/3rd emitted insn, before the divisions).
 *  - `tools/rtlguide.py`: names the same hunk (`lw a0,84(sp)` vs target's
 *    late `lw v0,92(sp)`) and classifies the dominant cause as
 *    structure/length (6) over regalloc (2) — i.e. this is not a pure
 *    allocator coin-flip, but neither rtlguide nor two direct experiments
 *    (below) found the specific source difference that would fix it.
 *  - `tools/permute.py`: self-diagnoses and refuses — "uses the gte.h macro
 *    layer — the permuter's C parser rejects inline asm and cannot search
 *    this class at all" — the same tool limitation FUN_80058c70 hit.
 *  - `tools/autorules.py`: 27 mechanical rules tried, best 620→620 (no win)
 *    — matches FUN_80058c70's own "26 candidates, all >= 620".
 *  - Two direct structural experiments here, both WORSE (falsifying the
 *    obvious fixes): hoisting `puVar4 = param_1 + 5;` unconditionally to
 *    the top (to keep $a0's "real" use textually early) shortened the
 *    function to 916 bytes — LENGTH MISMATCH, a regression, not a fix.
 *    Reordering the param_5/param_6 read order (param_7[3]=param_5 before
 *    uVar3=*(param_6+4), the reverse of FUN_80058c70's already-tuned order)
 *    stayed correct length but rose to 637/920 — also worse. Every
 *    structural perturbation tried on this shape (these two, plus
 *    FUN_80058c70's own three: dropping local_38 (888), swapping the
 *    puVar5/local_38 roles (924), hoisting `local_38 = puVar5` above the
 *    guard (924)) breaks LENGTH or worsens the byte count — the transcribed
 *    structure is a narrow local optimum, not an arbitrary starting guess.
 *
 * Falsify this park by finding a source change that keeps param_1 resident
 * in $a0 across the prologue without changing observable behavior or
 * function length — that is the one lever nothing above has hit.
 */

extern int HWD0;
extern int VWD0;
extern void FUN_80057b80(u_long *outv, u_long *packet, int mode);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80059008", FUN_80059008);
#else
u_long *FUN_80059008(u_short *param_1, u_long param_2, u_long *param_3, int param_4,
                     u_long param_5, u_long param_6, u_long *param_7)
{
    int iVar1;
    int iVar2;
    u_long uVar3;
    int iVar3;
    SVECTOR *r0;
    u_short *puVar4;
    u_long *puVar5;
    u_char uVar6;
    SVECTOR *r0_00;
    SVECTOR *r2;
    SVECTOR *r1;
    u_long *local_38;

    iVar1 = HWD0;
    *param_7 = 4;
    iVar2 = VWD0;
    puVar5 = param_7 + 0x38;
    *(short *)(param_7 + 0xd) = (short)(iVar1 / 2);
    *(short *)((int)param_7 + 0x36) = (short)(iVar2 / 2);
    uVar3 = *(u_long *)(param_6 + 4);
    param_7[8] = 0x96;
    *(u_char *)((int)param_7 + 0x4f) = 0xc;
    param_7[3] = param_5;
    param_7[5] = (u_long)param_3;
    *(u_char *)((int)param_7 + 0x53) = 0x3c;
    param_7[4] = uVar3;
    if (param_4 != 0) {
        r0 = (SVECTOR *)(param_7 + 0x20);
        r1 = (SVECTOR *)(param_7 + 0x26);
        r2 = (SVECTOR *)(param_7 + 0x2c);
        r0_00 = (SVECTOR *)(param_7 + 0x32);
        iVar3 = 0x3c;
        puVar4 = param_1 + 5;
        local_38 = puVar5;
    loop_top:
            *(short *)(param_7 + 0x20) = *(u_short *)(puVar4[7] * 8 + param_2);
            *(short *)((int)param_7 + 0x82) = *(u_short *)(puVar4[7] * 8 + param_2 + 2);
            *(short *)(param_7 + 0x21) = *(u_short *)(puVar4[7] * 8 + param_2 + 4);
            *(short *)(param_7 + 0x26) = *(u_short *)(puVar4[8] * 8 + param_2);
            *(short *)((int)param_7 + 0x9a) = *(u_short *)(puVar4[8] * 8 + param_2 + 2);
            *(short *)(param_7 + 0x27) = *(u_short *)(puVar4[8] * 8 + param_2 + 4);
            *(short *)(param_7 + 0x2c) = *(u_short *)(puVar4[9] * 8 + param_2);
            *(short *)((int)param_7 + 0xb2) = *(u_short *)(puVar4[9] * 8 + param_2 + 2);
            *(short *)(param_7 + 0x2d) = *(u_short *)(puVar4[9] * 8 + param_2 + 4);
            *(short *)(param_7 + 0x32) = *(u_short *)(puVar4[10] * 8 + param_2);
            *(short *)((int)param_7 + 0xca) = *(u_short *)(puVar4[10] * 8 + param_2 + 2);
            *(short *)(param_7 + 0x33) = *(u_short *)(puVar4[10] * 8 + param_2 + 4);
            *puVar5 = (u_long)r0;
            puVar5[1] = (u_long)r1;
            puVar5[2] = (u_long)r2;
            puVar5[3] = (u_long)r0_00;
            gte_ldv3(r0, r1, r2);
            gte_rtpt();
            *(short *)(param_7 + 0x25) = puVar4[-3];
            *(short *)(param_7 + 0x2b) = puVar4[-1];
            gte_stsxy3(param_7 + 0x23, param_7 + 0x29, param_7 + 0x2f);
            gte_nclip();
            *(short *)(param_7 + 0x31) = puVar4[1];
            *(short *)(param_7 + 0x37) = puVar4[3];
            gte_stopz(param_7 + 6);
            if (0 < (int)param_7[6]) {
                gte_ldv0(r0_00);
                gte_rtps();
                param_7[0x22] = *(u_long *)(puVar4 + 5);
                uVar6 = (u_char)iVar3;
                *(u_char *)((int)param_7 + 0x8b) = uVar6;
                param_7[0x28] = *(u_long *)(puVar4 + 5);
                *(u_char *)((int)param_7 + 0xa3) = uVar6;
                param_7[0x2e] = *(u_long *)(puVar4 + 5);
                *(u_char *)((int)param_7 + 0xbb) = uVar6;
                param_7[0x34] = *(u_long *)(puVar4 + 5);
                *(u_char *)((int)param_7 + 0xd3) = uVar6;
                gte_stsxy(param_7 + 0x35);
                gte_stsz4(param_7 + 0x24, param_7 + 0x2a, param_7 + 0x30, param_7 + 0x36);
                *(short *)((int)param_7 + 0x5a) = puVar4[-2];
                *(short *)((int)param_7 + 0x66) = *puVar4;
                FUN_80057b80(local_38, param_7, 0);
            }
            param_4 = param_4 + -1;
            puVar4 = puVar4 + 0x10;
            if (param_4 != 0) goto loop_top;
    }
    return (u_long *)param_7[5];
}
#endif
