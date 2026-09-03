#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDdivF4LFG[]; /* "TMDdivF4LFG\n" */
extern s32 warn_dmyTMDdivF4LFG;

void *dmyGsTMDdivF4LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivF4LFG == 0)
    {
        printf(str_dmyTMDdivF4LFG);
        warn_dmyTMDdivF4LFG = 1;
    }
    return arg3;
}
