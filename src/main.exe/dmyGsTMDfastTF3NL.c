#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastTF3NL[]; /* "TMDfastTF3NL\n" */
extern s32 warn_dmyTMDfastTF3NL;

PACKET *dmyGsTMDfastTF3NL(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastTF3NL == 0)
    {
        printf(str_dmyTMDfastTF3NL);
        warn_dmyTMDfastTF3NL = 1;
    }
    return packet;
}
