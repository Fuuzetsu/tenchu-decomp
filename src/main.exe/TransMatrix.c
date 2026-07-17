#include "common.h"
#include "main.exe.h"

/*
 * TransMatrix (0x80078354, 36 bytes) — stock PsyQ libgte (Ghidra places it in
 * MTX_07.OBJ): set a MATRIX's translation vector from a VECTOR, return the
 * matrix. `t` is at offset 20, after the 3x3 short rotation block.
 *
 * STATUS: NON_MATCHING at 23/36 — and the evidence says this is a HANDWRITTEN-ASM
 * original, not a tie. FLAGGED FOR OWNER DECISION: it is not yet in
 * config/handwritten-asm.txt, and adding it is an owner call (that list is an
 * owner decision, docs/gte-policy.md). Two independent cc1-invariant tells, both
 * measured here, not argued:
 *
 * TARGET                       OURS (the 3-temp draft below)
 *   lw   t0,0(a1)                lw   v1,0(a1)
 *   lw   t1,4(a1)                lw   a0,4(a1)
 *   lw   t2,8(a1)                lw   a1,8(a1)
 *   sw   t0,20(a0)               sw   v1,20(v0)
 *   sw   t1,24(a0)               sw   a0,24(v0)
 *   sw   t2,28(a0)               sw   a1,28(v0)
 *   move v0,a0                   jr   ra
 *   jr   ra                      sw   a1,28(v0)   <- slot FILLED
 *   nop                        (8 insns / 32 bytes vs the target's 9 / 36)
 *
 * 1. REGISTER CHOICE. The draft reproduces the target's SHAPE exactly — three
 *    loads held live, then three stores, which is the "N adjacent loads are
 *    source temps" rule working. But cc1 colours them v1(3), a0(4), a1(5): the
 *    numerically lowest free registers, reusing the two parameter registers the
 *    moment they die. The target uses t0(8), t1(9), t2(10), stepping over four
 *    lower registers. config/mips/mips.h defines no REG_ALLOC_ORDER (0 hits), and
 *    BOTH allocators then walk the hard registers NUMERICALLY — verified in the
 *    pinned source, not recalled (`tools/ccsrc.py --grep REG_ALLOC_ORDER
 *    --context 4`): global.c:960 and local-alloc.c:2266 carry the identical
 *    `#ifdef REG_ALLOC_ORDER / regno = reg_alloc_order[i]; #else / regno = i;`.
 *    So it does not matter whether these temps are coloured by local_alloc or
 *    global_alloc: there is no allocation this compiler performs that lands on
 *    t0/t1/t2 while v1/a0/a1 sit free.
 * 2. AN UNFILLED DELAY SLOT WITH AN OBVIOUS FILLER. The target ends
 *    `move v0,a0 / jr ra / nop`. `move v0,a0` is independent of the jump and sits
 *    directly above it; reorg fills that slot every time — our build does exactly
 *    that, which is the whole 4-byte length difference. No C makes cc1 decline.
 *
 * Same class as drawF3 and the draw* family: an original whose asm IS the faithful
 * source. Historically consistent too — PsyQ's libgte matrix primitives were
 * hand-written assembly. If the owner agrees, add TransMatrix to
 * config/handwritten-asm.txt; progress.py will then count it as done-by-asm and
 * triage/matcher-prompt will refuse it as a target (matcher-prompt refuses that
 * class since d36384f). Until then it stays parked and honest.
 *
 * WORTH CHECKING BEFORE THE SDK PUSH: if libgte's primitives are broadly
 * hand-written, the "959 SDK functions remaining" figure in PLAN.md overstates
 * what is actually matchable, and the SDK plan should establish the boundary
 * first rather than discover it one function at a time.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/TransMatrix", TransMatrix);
#else
MATRIX *TransMatrix(MATRIX *m, VECTOR *v)
{
    long x = v->vx;
    long y = v->vy;
    long z = v->vz;

    m->t[0] = x;
    m->t[1] = y;
    m->t[2] = z;
    return m;
}
#endif
