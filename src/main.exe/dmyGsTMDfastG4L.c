#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastG4L[]; /* "TMDfastG4L\n" */
extern s32 warn_dmyTMDfastG4L;

PACKET *dmyGsTMDfastG4L(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastG4L == 0)
    {
        printf(str_dmyTMDfastG4L);
        warn_dmyTMDfastG4L = 1;
    }
    return packet;
}
