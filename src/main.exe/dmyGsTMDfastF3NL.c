#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastF3NL[]; /* "TMDfastF3NL\n" */
extern s32 warn_dmyTMDfastF3NL;

void *dmyGsTMDfastF3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastF3NL == 0)
    {
        printf(str_dmyTMDfastF3NL);
        warn_dmyTMDfastF3NL = 1;
    }
    return arg3;
}
