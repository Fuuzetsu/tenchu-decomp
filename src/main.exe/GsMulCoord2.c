#include "common.h"
#include "main.exe.h"

/*
 * GsMulCoord2 (0x80065280) — stock PsyQ libgs: compose two coordinate matrices.
 * Rotate m2's translation through m1 into a local VECTOR, multiply the rotation
 * parts, then add m1's translation back in component-wise. Both matrices are
 * cached in callee-saved registers across the two calls.
 */
extern void ApplyMatrixLV(MATRIX *m, VECTOR *v0, VECTOR *v1);
extern void MulMatrix2(MATRIX *m0, MATRIX *m1);

void GsMulCoord2(MATRIX *m1, MATRIX *m2)
{
    VECTOR v;

    ApplyMatrixLV(m1, (VECTOR *)m2->t, &v);
    MulMatrix2(m1, m2);
    m2->t[0] = v.vx + m1->t[0];
    m2->t[1] = v.vy + m1->t[1];
    m2->t[2] = v.vz + m1->t[2];
}
