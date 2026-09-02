#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivF4NL[]; /* "TMDdivF4NL\n" */
extern s32 warn_dmyTMDdivF4NL;

void *dmyGsTMDdivF4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivF4NL == 0)
    {
        printf(str_dmyTMDdivF4NL);
        warn_dmyTMDdivF4NL = 1;
    }
    return arg3;
}
