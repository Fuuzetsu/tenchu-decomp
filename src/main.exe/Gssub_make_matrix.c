#include "common.h"
#include "main.exe.h"

void Gssub_make_matrix(MATRIX *m, short sine, short cosine, char axis)
{
    *m = GsIDMATRIX;

    switch (axis)
    {
    case 'X':
    case 'x':
        m->m[1][1] = cosine;
        m->m[2][2] = cosine;
        m->m[1][2] = -sine;
        m->m[2][1] = sine;
        break;

    case 'Y':
    case 'y':
        m->m[0][0] = cosine;
        m->m[2][2] = cosine;
        m->m[0][2] = sine;
        m->m[2][0] = -sine;
        break;

    case 'Z':
    case 'z':
        m->m[0][0] = cosine;
        m->m[1][1] = cosine;
        m->m[0][1] = -sine;
        m->m[1][0] = sine;
        break;
    }
}
