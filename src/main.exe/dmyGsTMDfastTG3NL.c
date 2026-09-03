#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastTG3NL[]; /* "TMDfastTG3NL\n" */
extern s32 warn_dmyTMDfastTG3NL;

PACKET *dmyGsTMDfastTG3NL(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastTG3NL == 0)
    {
        printf(str_dmyTMDfastTG3NL);
        warn_dmyTMDfastTG3NL = 1;
    }
    return packet;
}
