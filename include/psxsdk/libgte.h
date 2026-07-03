#ifndef LIBGTE_H
#define LIBGTE_H

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

#endif