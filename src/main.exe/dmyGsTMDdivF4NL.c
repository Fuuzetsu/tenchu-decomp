#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivF4NL (0x80067524) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
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
