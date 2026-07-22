#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetLightning(struct VECTOR *start, struct VECTOR *end, short r, short g, int b);
 *     EFFECT.C:1562, 3 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     param $a0       struct VECTOR * start
 *     param $a1       struct VECTOR * end
 *     param $a2       short r
 *     param $a3       short g
 *     param stack+16  int b
 * END PSX.SYM */

/*
 * SetLightning (0x80038f98, 0x44 bytes) — thin forwarder to SetLightningI,
 * inserting a constant `1` and re-widening its own narrow params.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - The shared retail API retains the original `r`/`g`/`b` names. The demo
 *    still had an `int b`, but retail's three trailing params (r/g
 *    register-passed, b stack-passed) get the IDENTICAL sll-16/sra-16
 *    re-sign-extend, because cc1 mechanically re-widens ANY
 *    narrow parameter on its first use regardless of class — a genuine
 *    `int` stack parameter that's merely narrowed for the callee instead
 *    folds straight to one `lh` (combine sees "load then keep low 16 bits"
 *    as one instruction, since the parm is just a MEM and a same-sized
 *    local copy doesn't stop this — copy-propagation erases the copy
 *    first). The tell: a full `lw` + sll/sra on a stack parameter used
 *    exactly once, narrowed exactly once, with NO other use, means the
 *    param's OWN type is narrow (mechanical widen-on-use), not `int`
 *    truncated-for-the-call (which would compile shorter, as `lh`).
 */
extern void SetLightningI(VECTOR *start, VECTOR *end, int gen,
                          short r, short g, short b);

void SetLightning(VECTOR *start, VECTOR *end, short r, short g, short b)
{
    SetLightningI(start, end, 1, r, g, b);
}
