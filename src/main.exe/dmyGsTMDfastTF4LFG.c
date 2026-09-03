#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern char str_dmyTMDfastTF4LFG[]; /* "TMDfastTF4LFG\n" */
extern s32 warn_dmyTMDfastTF4LFG;

PACKET *dmyGsTMDfastTF4LFG(void *primitive, VERT *vertices, SVECTOR *normals,
                PACKET *packet)
{
    if (warn_dmyTMDfastTF4LFG == 0)
    {
        printf(str_dmyTMDfastTF4LFG);
        warn_dmyTMDfastTF4LFG = 1;
    }
    return packet;
}
