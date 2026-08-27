#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastTG4NL (0x800684e4) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastTG4NL[]; /* "TMDfastTG4NL\n" */
extern s32 warn_dmyTMDfastTG4NL;

void *dmyGsTMDfastTG4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTG4NL == 0)
    {
        printf(str_dmyTMDfastTG4NL);
        warn_dmyTMDfastTG4NL = 1;
    }
    return arg3;
}
