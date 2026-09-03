#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastF3NL[]; /* "TMDfastF3NL\n" */
extern s32 warn_dmyTMDfastF3NL;

PACKET *dmyGsTMDfastF3NL(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastF3NL == 0)
    {
        printf(str_dmyTMDfastF3NL);
        warn_dmyTMDfastF3NL = 1;
    }
    return packet;
}
