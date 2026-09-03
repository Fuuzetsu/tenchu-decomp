#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastTF4L[]; /* "TMDfastTF4L\n" */
extern s32 warn_dmyTMDfastTF4L;

PACKET *dmyGsTMDfastTF4L(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastTF4L == 0)
    {
        printf(str_dmyTMDfastTF4L);
        warn_dmyTMDfastTF4L = 1;
    }
    return packet;
}
