#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetLightning(struct VECTOR *start, struct VECTOR *end, short r, short g, int b);
 *     EFFECT.C:1562, 3 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
