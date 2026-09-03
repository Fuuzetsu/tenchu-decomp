#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastG4LFG[]; /* "TMDfastG4LFG\n" */
extern s32 warn_dmyTMDfastG4LFG;

PACKET *dmyGsTMDfastG4LFG(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastG4LFG == 0)
    {
        printf(str_dmyTMDfastG4LFG);
        warn_dmyTMDfastG4LFG = 1;
    }
    return packet;
}
