#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ArrangeLocalMatrix(struct ModelType *model, struct MATRIX *t);
 *     ITEM.C:3328, 38 src lines, frame 72 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $a0       struct ModelType * model
 *     param $s3       struct MATRIX * t
 *     stack sp+16     struct MATRIX m
 *     reg   $a1       int i
 *     reg   $t4       int j
 *     reg   $t5       int k
 *     reg   $s1       long det
 *     reg   $a3       long t
 *     reg   $t1       long u
 * END PSX.SYM */

/*
 * MATCH.
 *
 * This is a fixed-point Gauss-Jordan pass over the local 3x3 matrix.  Each
 * pivot row is normalised, the pivot column is eliminated from the other
 * rows, and a determinant-like scale check decides whether to compose and
 * publish the result.
 *
 * Matching notes:
 *  - The outer `i` loop and middle `j` loop are explicit top-tested
 *    `while (1)` loops.  The two `k` loops stay ordinary bottom-tested loops;
 *    that accounts for the target's two extra guard/jump pairs.
 *  - The first row-normalisation loop deliberately reuses `k`, as does the
 *    deepest elimination loop.  Retail therefore gives both counters $a2,
 *    while the middle `j` loop stays in $t4.  Using `j` for both adjacent
 *    loops preserves the values but rotates nine caller-saved registers.
 *  - Spelling the deepest test as `k != i` puts the multiply/subtract path
 *    first and the variable division later.  The opposite equivalent test is
 *    one instruction short because its jump delay slot hides the divide
 *    result's hazard gap.
 *  - The final whole-MATRIX assignment expands to the target's eight-word
 *    copy.  Variable divisions require maspsx's `--expand-div` compatibility
 *    pass to reproduce ASPSX's guarded `div` sequences.
 */

void ArrangeLocalMatrix(ModelType *model, MATRIX *t)
{
    enum
    {
        n = 3
    };
    MATRIX m;
    s32 i;
    s32 j;
    s32 k;
    s32 det;

    GsGetLw(&model->locate, &m);
    det = 0x1000;

    i = 0;
    while (1)
    {
        s32 t;

        if (i >= n)
        {
            break;
        }
        t = m.m[i][i];
        if (t == 0)
        {
            t = 0x1000;
        }
        det = det * t / 0x1000;

        for (k = 0; k < n; k++)
        {
            m.m[i][k] = m.m[i][k] * 0x1000 / t;
        }
        m.m[i][i] = 0x1000000 / t;

        j = 0;
        while (1)
        {
            if (j >= n)
            {
                break;
            }
            if (j != i)
            {
                s32 u;

                u = m.m[j][i];
                for (k = 0; k < n; k++)
                {
                    if (k != i)
                    {
                        m.m[j][k] -= m.m[i][k] * u / 0x1000;
                    }
                    else
                    {
                        m.m[j][i] = (-u * 0x1000) / t;
                    }
                }
            }
            j++;
        }
        i++;
    }

    if ((u32)(det - 0x800) < 0x801)
    {
        MulMatrix(&m, t);
        *t = m;
    }
}
