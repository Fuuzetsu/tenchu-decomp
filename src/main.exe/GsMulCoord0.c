#include "common.h"
#include "main.exe.h"

/*
 * GsMulCoord0 (0x800651f0) — stock PsyQ libgs: the three-matrix form of the
 * family. Unlike GsMulCoord2/3 it needs no local VECTOR — it rotates m2's
 * translation through m1 straight into m3's translation slot, composes into m3,
 * then adds m1's translation back in place.
 */

void GsMulCoord0(MATRIX *m1, MATRIX *m2, MATRIX *m3)
{
    ApplyMatrixLV(m1, (VECTOR *)m2->t, (VECTOR *)m3->t);
    MulMatrix0(m1, m2, m3);
    m3->t[0] = m3->t[0] + m1->t[0];
    m3->t[1] = m3->t[1] + m1->t[1];
    m3->t[2] = m3->t[2] + m1->t[2];
}
