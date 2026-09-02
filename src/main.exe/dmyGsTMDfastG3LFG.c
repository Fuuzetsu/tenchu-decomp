#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastG3LFG[]; /* "TMDfastG3LFG\n" */
extern s32 warn_dmyTMDfastG3LFG;

void *dmyGsTMDfastG3LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastG3LFG == 0)
    {
        printf(str_dmyTMDfastG3LFG);
        warn_dmyTMDfastG3LFG = 1;
    }
    return arg3;
}
