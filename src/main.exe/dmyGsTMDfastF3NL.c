#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastF3NL (0x80067d04) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastF3NL[]; /* "TMDfastF3NL\n" */
extern s32 warn_dmyTMDfastF3NL;

void *dmyGsTMDfastF3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastF3NL == 0)
    {
        printf(str_dmyTMDfastF3NL);
        warn_dmyTMDfastF3NL = 1;
    }
    return arg3;
}
