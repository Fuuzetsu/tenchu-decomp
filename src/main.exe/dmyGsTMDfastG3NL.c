#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastG3NL[]; /* "TMDfastG3NL\n" */
extern s32 warn_dmyTMDfastG3NL;

PACKET *dmyGsTMDfastG3NL(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastG3NL == 0)
    {
        printf(str_dmyTMDfastG3NL);
        warn_dmyTMDfastG3NL = 1;
    }
    return packet;
}
