#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastF4L[]; /* "TMDfastF4L\n" */
extern s32 warn_dmyTMDfastF4L;

PACKET *dmyGsTMDfastF4L(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastF4L == 0)
    {
        printf(str_dmyTMDfastF4L);
        warn_dmyTMDfastF4L = 1;
    }
    return packet;
}
