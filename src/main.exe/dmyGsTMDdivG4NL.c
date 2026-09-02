#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivG4NL[]; /* "TMDdivG4NL\n" */
extern s32 warn_dmyTMDdivG4NL;

void *dmyGsTMDdivG4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivG4NL == 0)
    {
        printf(str_dmyTMDdivG4NL);
        warn_dmyTMDdivG4NL = 1;
    }
    return arg3;
}
