#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivTG4NL[]; /* "TMDdivTG4NL\n" */
extern s32 warn_dmyTMDdivTG4NL;

void *dmyGsTMDdivTG4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivTG4NL == 0)
    {
        printf(str_dmyTMDdivTG4NL);
        warn_dmyTMDdivTG4NL = 1;
    }
    return arg3;
}
