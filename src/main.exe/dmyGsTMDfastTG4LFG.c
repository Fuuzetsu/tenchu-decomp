#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastTG4LFG[]; /* "TMDfastTG4LFG\n" */
extern s32 warn_dmyTMDfastTG4LFG;

PACKET *dmyGsTMDfastTG4LFG(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastTG4LFG == 0)
    {
        printf(str_dmyTMDfastTG4LFG);
        warn_dmyTMDfastTG4LFG = 1;
    }
    return packet;
}
