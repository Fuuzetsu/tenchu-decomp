#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivG4NL (0x80067764) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
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
