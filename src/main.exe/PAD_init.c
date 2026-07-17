#include "common.h"
#include "main.exe.h"

/*
 * PAD_init (0x800833b0) — stock PsyQ libapi: tear down any existing pad
 * handler, patch the pad ISR under a critical section, hand the caller's four
 * buffers to PAD_init2, and record that the pad system is up.
 *
 * All four parameters are cached in callee-saved registers across the six
 * argument-less calls, then replayed into PAD_init2.
 *
 * InitPAD (0x80083440) is the SAME function with one instruction different —
 * its final `jal` goes to 0x80083694 instead of PAD_init2 — so it is a
 * near-clone, not a separate job. Match this one and clone it.
 *
 * STATUS: NON_MATCHING at 26/144. **Every instruction is correct and every
 * operand is correct — the residual is pure EMISSION ORDER in the tail**, and it
 * is one contiguous swap:
 *
 *   TARGET                    OURS
 *     li   v0,1                 lw ra,32(sp)
 *     lui  at,0x8009            lw s3,28(sp)
 *     sw   v0,30168(at)         lw s2,24(sp)
 *     lw   ra,32(sp)            lw s1,20(sp)
 *     lw   s3,28(sp)            lw s0,16(sp)
 *     lw   s2,24(sp)            li v0,1
 *     lw   s1,20(sp)            lui at,0x8009
 *     lw   s0,16(sp)            sw v0,30168(at)
 *     jr   ra                   jr ra
 *     addiu sp,sp,40            addiu sp,sp,40
 *
 * The target does `D_800975D8 = 1;` BEFORE the callee-saved restores — the
 * natural body-then-epilogue order. We sink it BELOW them. The restores are
 * frame loads and the store is to a symbol_ref, so cc1 can prove they do not
 * alias and is free to reorder; the question is why it does here and did not
 * there.
 *
 * MEASURED, not argued (`tools/sched-deps.py PAD_init --pass sched2`, whose
 * reconstruction self-validates against cc1's own post-pass chain):
 *   - Final order idx 22..26 are the restores (insns 71,73,75,77,79), idx 27..28
 *     are `li v0,1` + the store (insns 47,49), idx 29 the blockage (80).
 *   - ALL of them print priority 1, so the sort falls through to LUID DESC.
 *     Note priority 1 here means UNIT LATENCY, not "no producers"
 *     (docs/compiler-facts.md) — do not re-derive the old depth story.
 *   - sched2 cannot be bumping anything: adjust_priority is gated on
 *     `reload_completed == 0`, so in sched2 the priority column is exact.
 *
 * TRIED AND MEASURED:
 *   - `extern volatile s32 D_800975D8;` — a NO-OP (still 26). The intuition was
 *     that a volatile store would forbid hoisting loads across it, and that a
 *     pad-up flag read by the ISR `_patch_pad` installs is a principled reason
 *     for the qualifier. It changes nothing here. Do not re-try it.
 *
 * NEXT: this is a LUID question, so the lever is pre-sched ORDER, not priority.
 * Read `sched-deps --pass sched2`'s idx column against the pre-sched chain and
 * find what puts the store's LUID above the restores'. Do NOT hand-read the raw
 * `;; ready list` lines — a hazard cycle prints `, now` twice and the first
 * head is the loser (docs/compiler-facts.md).
 */
extern void _remove_ChgclrPAD(void);
extern void EnterCriticalSection(void);
extern void _patch_pad(void);
extern void ExitCriticalSection(void);
extern void ChangeClearPAD(long mode);
extern void FUN_80083538(void);
extern void PAD_init2(u_long a, u_long b, u_long c, u_long d);

extern s32 D_800975D8;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PAD_init", PAD_init);
#else
void PAD_init(u_long a, u_long b, u_long c, u_long d)
{
    _remove_ChgclrPAD();
    EnterCriticalSection();
    _patch_pad();
    ExitCriticalSection();
    ChangeClearPAD(0);
    FUN_80083538();
    PAD_init2(a, b, c, d);
    D_800975D8 = 1;
}
#endif
