#include "common.h"
#include "main.exe.h"

void GsMulCoord3(MATRIX *m1, MATRIX *m2)
{
    VECTOR v;

    ApplyMatrixLV(m1, (VECTOR *)m2->t, &v);
    MulMatrix(m1, m2);
    m1->t[0] = v.vx + m1->t[0];
    m1->t[1] = v.vy + m1->t[1];
    m1->t[2] = v.vz + m1->t[2];
}
