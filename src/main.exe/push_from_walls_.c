#include "common.h"
#include "main.exe.h"

void push_from_walls_(VECTOR *pos, s32 amount)
{
    MapVector v1;
    MapVector v2;
    s32 half;
    u32 vec;
    u16 xAdj;
    s32 signedX;
    u16 zAdj;

    GetAreaMapVector(GlobalAreaMap, &v1, pos, amount, AREA_LEVEL_DEFAULT);
    if (v1.level == LEVEL_NONE)
    {
        return;
    }
    if (v1.vector == 0)
    {
        return;
    }
    half = amount / 2;
    GetAreaMapVector(GlobalAreaMap, &v2, pos, half, AREA_LEVEL_DEFAULT);
    vec = v2.vector;
    if (vec == 0)
    {
        amount = half;
        vec = v1.vector;
    }
    xAdj = RefrectMove[vec][0];
    zAdj = RefrectMove[vec][1];
    if (xAdj != 0)
    {
        signedX = (s16)xAdj;
    }
    else
    {
        signedX = (s16)xAdj;
    }
    if (signedX > 0)
    {
        pos->vx += amount;
    }
    else if (signedX < 0)
    {
        pos->vx -= amount;
    }
    if ((s16)zAdj > 0)
    {
        pos->vz += amount;
    }
    else if ((s16)zAdj < 0)
    {
        pos->vz -= amount;
    }
}
