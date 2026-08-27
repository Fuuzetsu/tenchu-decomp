#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastG4NL (0x800682a4) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastG4NL[]; /* "TMDfastG4NL\n" */
extern s32 warn_dmyTMDfastG4NL;

void *dmyGsTMDfastG4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastG4NL == 0)
    {
        printf(str_dmyTMDfastG4NL);
        warn_dmyTMDfastG4NL = 1;
    }
    return arg3;
}
