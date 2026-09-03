#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastF4LFG[]; /* "TMDfastF4LFG\n" */
extern s32 warn_dmyTMDfastF4LFG;

void *dmyGsTMDfastF4LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastF4LFG == 0)
    {
        printf(str_dmyTMDfastF4LFG);
        warn_dmyTMDfastF4LFG = 1;
    }
    return arg3;
}
