#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastG4NL[]; /* "TMDfastG4NL\n" */
extern s32 warn_dmyTMDfastG4NL;

PACKET *dmyGsTMDfastG4NL(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastG4NL == 0)
    {
        printf(str_dmyTMDfastG4NL);
        warn_dmyTMDfastG4NL = 1;
    }
    return packet;
}
