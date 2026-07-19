#ifndef LIBGTE_H
#define LIBGTE_H

#include <types.h>

typedef struct VECTOR VECTOR;
struct VECTOR
{
    long vx, vy, vz;
    long pad;
};

typedef struct SVECTOR SVECTOR;
struct SVECTOR
{
    short vx, vy, vz;
    short pad;
};

typedef struct MATRIX MATRIX;
struct MATRIX
{
    short m[3][3];
    long t[3];
};

typedef struct CVECTOR CVECTOR;
struct CVECTOR
{
    u_char r, g, b, cd;
};

typedef struct DVECTOR DVECTOR;
struct DVECTOR
{
    short vx, vy;
};

/* Canonical PsyQ 4.5/4.6 LIBGTE declarations used by the game. */
MATRIX *MulMatrix0(MATRIX *m0, MATRIX *m1, MATRIX *m2);
MATRIX *MulMatrix(MATRIX *m0, MATRIX *m1);
MATRIX *MulMatrix2(MATRIX *m0, MATRIX *m1);
VECTOR *ApplyRotMatrix(SVECTOR *v0, VECTOR *v1);
VECTOR *ApplyMatrixLV(MATRIX *m, VECTOR *v0, VECTOR *v1);
SVECTOR *ApplyMatrixSV(MATRIX *m, SVECTOR *v0, SVECTOR *v1);
MATRIX *RotMatrix(SVECTOR *r, MATRIX *m);
MATRIX *RotMatrixYXZ(SVECTOR *r, MATRIX *m);
MATRIX *TransMatrix(MATRIX *m, VECTOR *v);
MATRIX *ScaleMatrix(MATRIX *m, VECTOR *v);
void SetRotMatrix(MATRIX *m);
void SetColorMatrix(MATRIX *m);
void SetTransMatrix(MATRIX *m);
void SetGeomScreen(long h);
long RotTransPers(SVECTOR *v0, long *sxy, long *p, long *flag);
void RotTrans(SVECTOR *v0, VECTOR *v1, long *flag);
long VectorNormalSS(SVECTOR *v0, SVECTOR *v1);
long SquareRoot0(long a);
int rcos(int a);
int rsin(int a);
long ratan2(long y, long x);

#endif
