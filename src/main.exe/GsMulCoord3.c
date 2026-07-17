#include "common.h"
#include "main.exe.h"

/*
 * GsMulCoord3 (0x800652fc) — stock PsyQ libgs, GsMulCoord2's sibling with two
 * deliberate differences (both verified in the asm, not assumed from the clone):
 *   - it composes via MulMatrix, not MulMatrix2;
 *   - it writes the summed translation back into m1 (`sw v0,20(s0)`), where
 *     GsMulCoord2 writes it into m2 (`sw v0,20(s1)`).
 * Otherwise identical: rotate m2's translation through m1 into a local VECTOR,
 * multiply, then add m1's own translation back component-wise.
 */
extern void ApplyMatrixLV(MATRIX *m, VECTOR *v0, VECTOR *v1);
extern void MulMatrix(MATRIX *m0, MATRIX *m1);

void GsMulCoord3(MATRIX *m1, MATRIX *m2)
{
    VECTOR v;

    ApplyMatrixLV(m1, (VECTOR *)m2->t, &v);
    MulMatrix(m1, m2);
    m1->t[0] = v.vx + m1->t[0];
    m1->t[1] = v.vy + m1->t[1];
    m1->t[2] = v.vz + m1->t[2];
}
