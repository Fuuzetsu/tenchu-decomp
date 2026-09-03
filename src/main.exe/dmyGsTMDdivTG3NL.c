#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDdivTG3NL[]; /* "TMDdivTG3NL\n" */
extern s32 warn_dmyTMDdivTG3NL;

void *dmyGsTMDdivTG3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivTG3NL == 0)
    {
        printf(str_dmyTMDdivTG3NL);
        warn_dmyTMDdivTG3NL = 1;
    }
    return arg3;
}
