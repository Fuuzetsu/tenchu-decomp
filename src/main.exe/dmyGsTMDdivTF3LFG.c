#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDdivTF3LFG[]; /* "TMDdivTF3LFG\n" */
extern s32 warn_dmyTMDdivTF3LFG;

void *dmyGsTMDdivTF3LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivTF3LFG == 0)
    {
        printf(str_dmyTMDdivTF3LFG);
        warn_dmyTMDdivTF3LFG = 1;
    }
    return arg3;
}
