#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastF3L[]; /* "TMDfastF3L\n" */
extern s32 warn_dmyTMDfastF3L;

PACKET *dmyGsTMDfastF3L(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastF3L == 0)
    {
        printf(str_dmyTMDfastF3L);
        warn_dmyTMDfastF3L = 1;
    }
    return packet;
}
