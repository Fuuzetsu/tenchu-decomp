#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastTF4NL[]; /* "TMDfastTF4NL\n" */
extern s32 warn_dmyTMDfastTF4NL;

PACKET *dmyGsTMDfastTF4NL(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastTF4NL == 0)
    {
        printf(str_dmyTMDfastTF4NL);
        warn_dmyTMDfastTF4NL = 1;
    }
    return packet;
}
