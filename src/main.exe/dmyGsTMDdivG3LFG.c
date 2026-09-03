#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDdivG3LFG[]; /* "TMDdivG3LFG\n" */
extern s32 warn_dmyTMDdivG3LFG;

void *dmyGsTMDdivG3LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivG3LFG == 0)
    {
        printf(str_dmyTMDdivG3LFG);
        warn_dmyTMDdivG3LFG = 1;
    }
    return arg3;
}
