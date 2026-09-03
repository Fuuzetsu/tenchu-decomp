#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastG3NL[]; /* "TMDfastG3NL\n" */
extern s32 warn_dmyTMDfastG3NL;

void *dmyGsTMDfastG3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastG3NL == 0)
    {
        printf(str_dmyTMDfastG3NL);
        warn_dmyTMDfastG3NL = 1;
    }
    return arg3;
}
