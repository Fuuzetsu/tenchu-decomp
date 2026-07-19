#ifndef LIBGTE_H
#define LIBGTE_H

#include <types.h>

/* Minimal ABI declarations; provenance and validation: docs/psyq-headers.md. */
typedef struct
{
    long vx, vy, vz;
    long pad;
} VECTOR;

typedef struct
{
    short vx, vy, vz;
    short pad;
} SVECTOR;

typedef struct
{
    short m[3][3];
    long t[3];
} MATRIX;

typedef struct
{
    u_char r, g, b, cd;
} CVECTOR;

typedef struct
{
    short vx, vy;
} DVECTOR;

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
void SetBackColor(long rbk, long gbk, long bbk);
void SetGeomScreen(long h);
long RotTransPers(SVECTOR *v0, long *sxy, long *p, long *flag);
void RotTrans(SVECTOR *v0, VECTOR *v1, long *flag);
long VectorNormalSS(SVECTOR *v0, SVECTOR *v1);
long SquareRoot0(long a);
int rcos(int a);
int rsin(int a);
long ratan2(long y, long x);

#endif
