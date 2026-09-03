#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastTG4NL[]; /* "TMDfastTG4NL\n" */
extern s32 warn_dmyTMDfastTG4NL;

void *dmyGsTMDfastTG4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTG4NL == 0)
    {
        printf(str_dmyTMDfastTG4NL);
        warn_dmyTMDfastTG4NL = 1;
    }
    return arg3;
}
