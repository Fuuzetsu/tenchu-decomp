#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastTG4NL[]; /* "TMDfastTG4NL\n" */
extern s32 warn_dmyTMDfastTG4NL;

PACKET *dmyGsTMDfastTG4NL(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastTG4NL == 0)
    {
        printf(str_dmyTMDfastTG4NL);
        warn_dmyTMDfastTG4NL = 1;
    }
    return packet;
}
