#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long CGetLevel(struct AreaNodeType **hint, long x, long y, long z, unsigned long flag);
 *     EFFECT.C:220, 34 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $a0       struct AreaNodeType ** hint
 *     param $a1       long x
 *     param $a2       long y
 *     param $a3       long z
 *     param stack+16  unsigned long flag
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct AreaNodeType *FieldArea;
 * END PSX.SYM */

/*
 * Matching notes (all verified against the original bytes):
 *  - `AreaNodeType` is CONFLICT.C's recovered
 *    `y/dy/x1/z1/x2/z2/attribute/division` layout. This EFFECT.C user keeps
 *    its own extern prototype spellings below where retail codegen requires
 *    them.
 *  - `GlobalAreaMap` is typed `unsigned long *` here (matching THIS TU's
 *    own PSX.SYM global declaration), not `NodeIndexType *` the way
 *    GetAreaMapLevel.c's OWN parameter is typed — cookbook: "Original TUs
 *    routinely disagree with the defining TU's prototype — respell
 *    per-TU, don't fix the shared header." `GetAreaMapLevel`'s 5th
 *    parameter is declared `short` here (matching the raw asm's
 *    sign-extending `sll/sra` narrowing of the stack-passed `flag`), even
 *    though its own TU types it `u16` — same per-TU respelling.
 *  - `x10`/`z10` get named locals (each used twice in the guard); `y/10`
 *    is written INLINE at both uses instead of through a matching named
 *    local — cc1 CSEs the repeated inline division into the target's
 *    single computation either way, and this spelling reproduces the
 *    exact register pairing (cookbook: "keep a compared value inline in
 *    both operands; hoisting it into a temp can swap two registers").
 *  - **Ghidra's polarity is backwards for this guard.** It renders
 *    `if (node != 0 && <6 conditions>) { foundBody } elseBody;` (found
 *    nested, else/GetAreaMapLevel as the tail) — literally true, but an
 *    `&&`-chain ALWAYS compiles with the conjunction's OWN body as the
 *    fallthrough right after the last test and the failure path reached
 *    by the merged `goto`s, which is the OPPOSITE of the target's layout
 *    (the GetAreaMapLevel/"skip" path is the fallthrough; the
 *    ComputeAreaLevel/"found" path is a forward jump target). De-Morgan
 *    the whole guard into an early-return block instead — `if (node == 0
 *    || <6 negated conditions>) { skipBody } else if (dy != 0) {
 *    foundBody... } else { ... }` — same individual comparisons (De
 *    Morgan on a chain of ANDed comparisons just flips each `<=`/`<` to
 *    its complement and turns `&&` into `||`, so the SIX test
 *    instructions are unaffected), but now the "guard fails" body is
 *    what falls through immediately after the tests, matching the
 *    target. (A plain `goto`-ladder with the SAME test polarity as the
 *    `&&`-chain does NOT change the layout either — cc1 lowers both
 *    spellings to identical RTL; only inverting which body is the "then"
 *    moves it.)
 *  - `node->dy != 0` and the `ComputeAreaLevel` call are NOT one
 *    `&&`-chained condition the way Ghidra's SSA shows it
 *    (`(dy!=0) && (level=..., level==MIN)`) — the real asm computes
 *    `ComputeAreaLevel` unconditionally once `dy != 0`, then separately
 *    tests the MIN-int sentinel; write the guard and the sentinel check
 *    as two nested statements, matching DrawHinoko.c's identical
 *    "not a comma-chained guard" note for the same shape.
 *  - **All three arms fold into ONE shared `ret` + ONE textual `return`.**
 *    With three separate `return expr;` statements (one per arm) cc1
 *    generates the epilogue's register-restore + `move v0,a0` sequence
 *    TWICE (once for the MIN early-out, once for the real end-of-function
 *    return) instead of recognizing they're byte-identical tails and
 *    sharing one — a genuine cross-jump/tail-merge miss in this cc1, not
 *    a semantic difference. Route the MIN case through `goto done;` to a
 *    single `done: return ret;` at the very end instead of a second
 *    `return` statement; every arm assigns the SAME `ret` local (not a
 *    per-arm `level`/`lv` split) so it lands in one persistent register
 *    (`$a0`) the whole way to that one `return`.
 */

extern long ComputeAreaLevel(AreaNodeType *node, long x, long z);
extern long GetAreaMapLevel(unsigned long *area, long x, long y, long z,
                            short mode);

long CGetLevel(AreaNodeType **hint, long x, long y, long z, unsigned long flag)
{
    long x10;
    long y10;
    long z10;
    AreaNodeType *node;
    long ret;

    x10 = x / 10;
    y10 = y / 10;
    node = *hint;
    z10 = z / 10;
    if (node == 0 || node->y - 200 > y10 || y10 > node->y || x10 < node->x1 ||
        z10 < node->z1 || node->x2 < x10 || node->z2 < z10)
    {
        ret = GetAreaMapLevel(GlobalAreaMap, x, y - 300, z, (short)flag);
        if (y <= ret && FieldArea->division == -1)
        {
            *hint = FieldArea;
        }
    }
    else if (node->dy != 0)
    {
        ret = ComputeAreaLevel(node, x10, z10);
        if (ret == 0x80000000)
        {
            goto done;
        }
        ret = ret * 10;
    }
    else
    {
        ret = node->y * 10;
    }
done:
    return ret;
}
