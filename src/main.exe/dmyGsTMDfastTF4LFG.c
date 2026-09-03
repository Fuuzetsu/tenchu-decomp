#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastTF4LFG[]; /* "TMDfastTF4LFG\n" */
extern s32 warn_dmyTMDfastTF4LFG;

void *dmyGsTMDfastTF4LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTF4LFG == 0)
    {
        printf(str_dmyTMDfastTF4LFG);
        warn_dmyTMDfastTF4LFG = 1;
    }
    return arg3;
}
