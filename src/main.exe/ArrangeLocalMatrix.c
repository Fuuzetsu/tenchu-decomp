#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ArrangeLocalMatrix(struct ModelType *model, struct MATRIX *t);
 *     ITEM.C:3328, 38 src lines, frame 72 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
        det = det * t / FIXED_ONE;

        for (k = 0; k < n; k++)
        {
            m.m[i][k] = m.m[i][k] * FIXED_ONE / t;
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
                        m.m[j][k] -= m.m[i][k] * u / FIXED_ONE;
                    }
                    else
                    {
                        m.m[j][i] = (-u * FIXED_ONE) / t;
                    }
                }
            }
            j++;
        }
        i++;
    }

    if (det >= 0x800 && det <= 0x1000)
    {
        MulMatrix(&m, t);
        *t = m;
    }
}
