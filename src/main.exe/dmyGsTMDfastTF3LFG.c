#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastTF3LFG[]; /* "TMDfastTF3LFG\n" */
extern s32 warn_dmyTMDfastTF3LFG;

void *dmyGsTMDfastTF3LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTF3LFG == 0)
    {
        printf(str_dmyTMDfastTF3LFG);
        warn_dmyTMDfastTF3LFG = 1;
    }
    return arg3;
}
