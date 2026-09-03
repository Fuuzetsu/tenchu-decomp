#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDdivG4LFG[]; /* "TMDdivG4LFG\n" */
extern s32 warn_dmyTMDdivG4LFG;

void *dmyGsTMDdivG4LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivG4LFG == 0)
    {
        printf(str_dmyTMDdivG4LFG);
        warn_dmyTMDdivG4LFG = 1;
    }
    return arg3;
}
