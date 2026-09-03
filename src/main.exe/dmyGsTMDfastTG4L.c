#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastTG4L[]; /* "TMDfastTG4L\n" */
extern s32 warn_dmyTMDfastTG4L;

PACKET *dmyGsTMDfastTG4L(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastTG4L == 0)
    {
        printf(str_dmyTMDfastTG4L);
        warn_dmyTMDfastTG4L = 1;
    }
    return packet;
}
