#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivG3NL (0x80066e64) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
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
