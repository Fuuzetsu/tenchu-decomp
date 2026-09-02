#include "common.h"
#include "main.exe.h"

void GsMulCoord0(MATRIX *m1, MATRIX *m2, MATRIX *m3)
{
    ApplyMatrixLV(m1, (VECTOR *)m2->t, (VECTOR *)m3->t);
    MulMatrix0(m1, m2, m3);
    m3->t[0] += m1->t[0];
    m3->t[1] += m1->t[1];
    m3->t[2] += m1->t[2];
}
