#ifndef LIBGS_H
#define LIBGS_H

#include <psxsdk/libgte.h>
#include <types.h>

typedef struct GsCOORD2PARAM GsCOORD2PARAM;

struct GsCOORD2PARAM
{
    VECTOR scale;
    SVECTOR rotate;
    VECTOR trans;
};

typedef struct GsCOORDINATE2 GsCOORDINATE2;
struct GsCOORDINATE2
{
    u_long flg;
    MATRIX coord;
    MATRIX workm;

    GsCOORD2PARAM *param;
    GsCOORDINATE2 *super;
    GsCOORDINATE2 *sub;
};

typedef struct GsDOBJ2 GsDOBJ2;

struct GsDOBJ2
{
    u_long attribute;
    GsCOORDINATE2 *coord2;
    u_long *tmd;
    u_long id;
};

#endif