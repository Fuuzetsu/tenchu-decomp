#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastF4NL[]; /* "TMDfastF4NL\n" */
extern s32 warn_dmyTMDfastF4NL;

void *dmyGsTMDfastF4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastF4NL == 0)
    {
        printf(str_dmyTMDfastF4NL);
        warn_dmyTMDfastF4NL = 1;
    }
    return arg3;
}
