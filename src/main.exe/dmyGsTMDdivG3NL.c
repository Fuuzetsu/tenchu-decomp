#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivG3NL[]; /* "TMDdivG3NL\n" */
extern s32 warn_dmyTMDdivG3NL;

void *dmyGsTMDdivG3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivG3NL == 0)
    {
        printf(str_dmyTMDdivG3NL);
        warn_dmyTMDdivG3NL = 1;
    }
    return arg3;
}
