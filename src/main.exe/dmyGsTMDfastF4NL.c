#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastF4NL[]; /* "TMDfastF4NL\n" */
extern s32 warn_dmyTMDfastF4NL;

PACKET *dmyGsTMDfastF4NL(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastF4NL == 0)
    {
        printf(str_dmyTMDfastF4NL);
        warn_dmyTMDfastF4NL = 1;
    }
    return packet;
}
