#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastTG3NL[]; /* "TMDfastTG3NL\n" */
extern s32 warn_dmyTMDfastTG3NL;

void *dmyGsTMDfastTG3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTG3NL == 0)
    {
        printf(str_dmyTMDfastTG3NL);
        warn_dmyTMDfastTG3NL = 1;
    }
    return arg3;
}
