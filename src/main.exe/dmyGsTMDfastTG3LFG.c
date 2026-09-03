#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastTG3LFG[]; /* "TMDfastTG3LFG\n" */
extern s32 warn_dmyTMDfastTG3LFG;

void *dmyGsTMDfastTG3LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTG3LFG == 0)
    {
        printf(str_dmyTMDfastTG3LFG);
        warn_dmyTMDfastTG3LFG = 1;
    }
    return arg3;
}
