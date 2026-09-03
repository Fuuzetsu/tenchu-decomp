#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastTG3LFG[]; /* "TMDfastTG3LFG\n" */
extern s32 warn_dmyTMDfastTG3LFG;

PACKET *dmyGsTMDfastTG3LFG(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastTG3LFG == 0)
    {
        printf(str_dmyTMDfastTG3LFG);
        warn_dmyTMDfastTG3LFG = 1;
    }
    return packet;
}
