#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastF4LFG[]; /* "TMDfastF4LFG\n" */
extern s32 warn_dmyTMDfastF4LFG;

PACKET *dmyGsTMDfastF4LFG(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastF4LFG == 0)
    {
        printf(str_dmyTMDfastF4LFG);
        warn_dmyTMDfastF4LFG = 1;
    }
    return packet;
}
