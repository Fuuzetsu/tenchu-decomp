#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastG3L[]; /* "TMDfastG3L\n" */
extern s32 warn_dmyTMDfastG3L;

PACKET *dmyGsTMDfastG3L(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastG3L == 0)
    {
        printf(str_dmyTMDfastG3L);
        warn_dmyTMDfastG3L = 1;
    }
    return packet;
}
