#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivTG4LFG[]; /* "TMDdivTG4LFG\n" */
extern s32 warn_dmyTMDdivTG4LFG;

void *dmyGsTMDdivTG4LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivTG4LFG == 0)
    {
        printf(str_dmyTMDdivTG4LFG);
        warn_dmyTMDdivTG4LFG = 1;
    }
    return arg3;
}
