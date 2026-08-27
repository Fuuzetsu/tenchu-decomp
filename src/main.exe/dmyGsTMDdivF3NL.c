#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivF3NL (0x80066c24) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivF3NL[]; /* "TMDdivF3NL\n" */
extern s32 warn_dmyTMDdivF3NL;

void *dmyGsTMDdivF3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivF3NL == 0)
    {
        printf(str_dmyTMDdivF3NL);
        warn_dmyTMDdivF3NL = 1;
    }
    return arg3;
}
