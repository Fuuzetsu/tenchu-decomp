#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastF3LFG[]; /* "TMDfastF3LFG\n" */
extern s32 warn_dmyTMDfastF3LFG;

PACKET *dmyGsTMDfastF3LFG(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastF3LFG == 0)
    {
        printf(str_dmyTMDfastF3LFG);
        warn_dmyTMDfastF3LFG = 1;
    }
    return packet;
}
