#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastG3LFG[]; /* "TMDfastG3LFG\n" */
extern s32 warn_dmyTMDfastG3LFG;

PACKET *dmyGsTMDfastG3LFG(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastG3LFG == 0)
    {
        printf(str_dmyTMDfastG3LFG);
        warn_dmyTMDfastG3LFG = 1;
    }
    return packet;
}
