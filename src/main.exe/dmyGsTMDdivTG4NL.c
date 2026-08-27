#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivTG4NL (0x80067be4) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
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
