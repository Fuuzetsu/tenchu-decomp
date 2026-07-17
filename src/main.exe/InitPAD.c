#include "common.h"
#include "main.exe.h"

/*
 * InitPAD (0x80083440) — stock PsyQ libapi: tear down any existing pad
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
 * MATCHED, and the shape is load-bearing — do not "clean it up".
 *
 * The `do {} while (0)` around the body is a LOOP-NOTE FENCE, and it is what puts
 * the `D_800975D8 = 1;` store BEFORE the callee-saved restores. Without it cc1
 * sinks the store below them (the restores are frame loads, the store targets a
 * symbol_ref, so it can prove they do not alias and reorders freely) — that was
 * the whole 26-byte residual. The `new_var` temp for the constant is part of the
 * same shape.
 *
 * `D_800975D8` must stay a plain `extern s32`, NOT `extern s32 D_800975D8[]`.
 * The plain form lets cc1 fold the address into the memory operand and emit ONE
 * insn, which `as` expands through the assembler temp — `lui at,0x8009 /
 * sw v0,30168(at)`, exactly the target. The array form forces the address into a
 * real register (`lui v0,0x8009 / sw v1,30168(v0)`) and costs 3 bytes.
 *
 * How it fell, because the route matters: the fence+temp came from a permuter
 * candidate (26 -> 3), and the last 3 bytes came from reverting the array form
 * the same candidate carried. Neither half is a match alone. The array form had
 * even looked like a win on its own (25 vs 26) while being a SHAPE regression —
 * see the negatives below.
 *
 * The near-clone of PAD_init (0x800833b0): identical but for its final `jal`,
 * which goes to InitPAD2 rather than PAD_init2. PAD_init's header carries the
 * full account of why the fence and the plain `extern s32` are load-bearing —
 * this file is a transcription of that matched shape, not an independent
 * derivation.
 *
 * MEASURED NEGATIVES (do not re-try):
 *   - `extern s32 D_800975D8[];` + `D_800975D8[0] = 1;` alone: 25, and further
 *     from the target's instruction shape than the 26 it replaced. Byte count is
 *     the score, not the goal.
 *   - `-mno-split-addresses` (the flag that rescued GS_107_OBJ_4B8): NO CHANGE.
 *     Reverted rather than left in — a per-TU compiler-input flag that does not
 *     earn its place is worse than none, and provenance alone does not justify it.
 *   - `extern volatile s32 D_800975D8;`: a no-op.
 */
extern void _remove_ChgclrPAD(void);
extern void EnterCriticalSection(void);
extern void _patch_pad(void);
extern void ExitCriticalSection(void);
extern void ChangeClearPAD(long mode);
extern void FUN_80083538(void);
extern void InitPAD2(u_long a, u_long b, u_long c, u_long d);

extern s32 D_800975D8;

void InitPAD(u_long a, u_long b, u_long c, u_long d)
{
    int new_var;

    _remove_ChgclrPAD();
    EnterCriticalSection();
    do {
        _patch_pad();
        ExitCriticalSection();
        ChangeClearPAD(0);
        FUN_80083538();
        InitPAD2(a, b, c, d);
        new_var = 1;
        D_800975D8 = new_var;
    } while (0);
}
