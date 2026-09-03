#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastF3GNL[]; /* "TMDfastF3GNL\n" */
extern s32 warn_dmyTMDfastF3GNL;

void *dmyGsTMDfastF3GNL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastF3GNL == 0)
    {
        printf(str_dmyTMDfastF3GNL);
        warn_dmyTMDfastF3GNL = 1;
    }
    return arg3;
}
