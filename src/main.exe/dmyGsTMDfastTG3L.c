#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastTG3L[]; /* "TMDfastTG3L\n" */
extern s32 warn_dmyTMDfastTG3L;

PACKET *dmyGsTMDfastTG3L(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastTG3L == 0)
    {
        printf(str_dmyTMDfastTG3L);
        warn_dmyTMDfastTG3L = 1;
    }
    return packet;
}
