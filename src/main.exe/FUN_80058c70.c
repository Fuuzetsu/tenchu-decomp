#include "common.h"
#include "main.exe.h"
#include "gte.h"

/*
 * FUN_80058c70 (0x80058c70, 0x398 bytes) — a DrawTMD-family fast primitive
 * renderer (TMD primitive code 0x3d), the sibling of the switch arms in
 * FUN_80058a54: it transforms each quad's four vertices through the GTE
 * (RTPT + a backface NCLIP + a per-quad RTPS) and assembles a POLY_GT4
 * (code 0x3c, len 0xc) packet, then hands the working state to FUN_80057b80.
 * Compiled-style GTE function under the restricted gte.h policy
 * (docs/gte-policy.md); the COP2 moves and GTE commands are the original
 * INLINE_N.H mechanism.
 *
 * STATUS: NON_MATCHING — 620 of 920 byte values differ (correct length,
 * 230/230 instructions). The default build keeps the byte-identical
 * INCLUDE_ASM stub; the #else draft is the reference reconstruction
 * (build with `NON_MATCHING=FUN_80058c70 ./Build`).
 *
 * The STRUCTURE is solved; the residual is a sub-C-level caller-saved
 * allocation tie. What is already byte-exact: the whole loop body, the
 * single cursor, the int counter, and every callee-saved role
 * (s0=param_7, s1=puVar4, s2=param_2, s4=param_4, s5=code, s6/s7/s8=
 * r0_00/r2/r1). Autopsy of how that was reached, and what blocks the rest:
 *
 * 1. GOTO LOOP, NOT do-while (the big one). A do-while makes loop.c
 *    strength-reduce: `.loop` reports biv 92 (puVar4, +44/iter) verified,
 *    all 20 array-access givs "combined with giv at 369" (add -4) and
 *    "reduced to (reg:SI 206)" — a SECOND induction register based at
 *    param_1+6 — while "Cannot eliminate biv 92: biv used in insn 374"
 *    (the bare `*puVar4`, add 0, is a direct biv use, never a giv). Result:
 *    two parallel cursors (param_1+6 and param_1+10), which cascades into a
 *    whole-function register shift (that was the old 591-byte draft, and it
 *    is a DEAD END). The target has NO reduction — it uses puVar4 itself
 *    (param_1+0xA) with plain offsets -6..+32 for every access, including
 *    offset 0. Per cookbook "No combined address bases => goto loops", the
 *    hand-rolled `loop_top: ... if (c) goto loop_top;` suppresses loop.c
 *    (no NOTE_INSN_LOOP_BEG) and reproduces the target's loop body exactly.
 *    Measured: goto=920 bytes (correct), do-while=948 bytes.
 *
 * 2. `int param_4`, not u16. Only valid WITH the goto loop. The target's
 *    `addiu s4,s4,-1; bnez s4` is unmasked, i.e. int. With a do-while, int
 *    strength-reduces (924 bytes) and u16 was the lesser evil (it masks:
 *    two extra `andi rX,0xffff`); with loop.c suppressed, int costs nothing
 *    and matches. Note the caller's extern says `u_short count` — the callee
 *    itself is int (ABI-compatible in $a3; the unmasked bnez proves it).
 *
 * 3. Prototype types come from the MATCHED caller FUN_80058a54:
 *    (u_short *primitive, u_long vertices, u_long *packet, count,
 *     u_long arg2, u_long arg1, u_long arg3). Ghidra's all-int guesses were
 *    wrong. Byte-neutral here, but faithful (param_1 is u_short*, so the
 *    cursor is `param_1 + 5`, not `(u_short*)(param_1 + 10)`).
 *
 * 4. uVar3 is TWO variables. Ghidra reused one name for the *(param_6+4)
 *    packet value and the 0x3c POLY_GT4 code; the target holds them in
 *    different registers ($a1 packet, $s5 code) and cc1 never splits a
 *    pseudo's live range, so one variable can never reach both. Split into
 *    uVar3 + iVar3 (622->620).
 *
 * BLOCKED ON (the 620): a caller-saved register-selection tie. All nine
 * callee-saved regs (s0-s8) are used by BOTH sides, so r0 must live in a
 * caller-saved reg in both; the target picks $a3, cc1 picks $t1. That is
 * driven by the entry `move t0,a0`: `.greg` shows "80 conflicts: ... 4 ...",
 * i.e. param_1 (reg 80) conflicts with $a0 because cc1 schedules param_6's
 * stack load into $a0 early (a0 is free before param_1's only use, the
 * preheader `puVar4 = param_1 + 5`). The target instead keeps param_1 in
 * $a0 and loads param_6 late into $v0 (free only after the HWD0/VWD0
 * divisions). This is circular — the register choice drives the schedule and
 * the schedule drives the register choice — and it cascades: param_1 in t0
 * leaves only t1 (not t0+a3) for the loop-invariant caller-saved values, so
 * puVar5/local_38 coalesce into one $s3 (target: $t0 + $s3 via `move s3,t0`)
 * and the frame comes out 64 instead of 72, shifting every sp-relative
 * offset. Tried and measured WORSE: dropping local_38 (888), swapping the
 * puVar5/local_38 store-vs-call-arg roles (924), hoisting `local_38 =
 * puVar5` above the guard (924, cc1 then reuses t1 for both and double-
 * spills). autorules finds nothing (26 candidates, all >= 620).
 * tools/permute.py CANNOT run on this function at all — its C parser rejects
 * the gte.h inline asm ("base.c does not contain any function!") — so the one
 * tool that normally settles a caller-saved tie is unavailable here.
 */

extern int HWD0;
extern int VWD0;
extern void FUN_80057b80(u_long *outv, u_long *packet, int mode);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80058c70", FUN_80058c70);
#else
u_long *FUN_80058c70(u_short *param_1, u_long param_2, u_long *param_3, int param_4,
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
            *(short *)(param_7 + 0x20) = *(u_short *)(puVar4[0xd] * 8 + param_2);
            *(short *)((int)param_7 + 0x82) = *(u_short *)(puVar4[0xd] * 8 + param_2 + 2);
            *(short *)(param_7 + 0x21) = *(u_short *)(puVar4[0xd] * 8 + param_2 + 4);
            *(short *)(param_7 + 0x26) = *(u_short *)(puVar4[0xe] * 8 + param_2);
            *(short *)((int)param_7 + 0x9a) = *(u_short *)(puVar4[0xe] * 8 + param_2 + 2);
            *(short *)(param_7 + 0x27) = *(u_short *)(puVar4[0xe] * 8 + param_2 + 4);
            *(short *)(param_7 + 0x2c) = *(u_short *)(puVar4[0xf] * 8 + param_2);
            *(short *)((int)param_7 + 0xb2) = *(u_short *)(puVar4[0xf] * 8 + param_2 + 2);
            *(short *)(param_7 + 0x2d) = *(u_short *)(puVar4[0xf] * 8 + param_2 + 4);
            *(short *)(param_7 + 0x32) = *(u_short *)(puVar4[0x10] * 8 + param_2);
            *(short *)((int)param_7 + 0xca) = *(u_short *)(puVar4[0x10] * 8 + param_2 + 2);
            *(short *)(param_7 + 0x33) = *(u_short *)(puVar4[0x10] * 8 + param_2 + 4);
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
                param_7[0x28] = *(u_long *)(puVar4 + 7);
                *(u_char *)((int)param_7 + 0xa3) = uVar6;
                param_7[0x2e] = *(u_long *)(puVar4 + 9);
                *(u_char *)((int)param_7 + 0xbb) = uVar6;
                param_7[0x34] = *(u_long *)(puVar4 + 0xb);
                *(u_char *)((int)param_7 + 0xd3) = uVar6;
                gte_stsxy(param_7 + 0x35);
                gte_stsz4(param_7 + 0x24, param_7 + 0x2a, param_7 + 0x30, param_7 + 0x36);
                *(short *)((int)param_7 + 0x5a) = puVar4[-2];
                *(short *)((int)param_7 + 0x66) = *puVar4;
                FUN_80057b80(local_38, param_7, 0);
            }
            param_4 = param_4 + -1;
            puVar4 = puVar4 + 0x16;
            if (param_4 != 0) goto loop_top;
    }
    return (u_long *)param_7[5];
}
#endif
