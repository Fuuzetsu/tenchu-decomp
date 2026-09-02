#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastG3GNL[]; /* "TMDfastG3GNL\n" */
extern s32 warn_dmyTMDfastG3GNL;

void *dmyGsTMDfastG3GNL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastG3GNL == 0)
    {
        printf(str_dmyTMDfastG3GNL);
        warn_dmyTMDfastG3GNL = 1;
    }
    return arg3;
}
