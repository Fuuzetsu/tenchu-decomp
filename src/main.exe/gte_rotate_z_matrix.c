#include "common.h"
#include "main.exe.h"

void gte_rotate_z_matrix(MATRIX *m, int angle)
{
    MATRIX rotation;
    int cosine;
    int sine;

    cosine = rcos(angle / 360);
    sine = rsin(angle / 360);

    if (angle != 0) {
        rotation.m[0][0] = cosine;
        rotation.m[0][1] = -sine;
        rotation.m[0][2] = 0;
        rotation.m[1][0] = sine;
        rotation.m[1][1] = cosine;
        rotation.m[1][2] = 0;
        rotation.m[2][0] = 0;
        rotation.m[2][1] = 0;
        rotation.m[2][2] = 0x1000;
        rotation.t[0] = 0;
        rotation.t[1] = 0;
        rotation.t[2] = 0;
        MulMatrix(m, &rotation);
    }
}
