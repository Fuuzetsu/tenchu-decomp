#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastTG3NL (0x80068064) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastTG3NL[]; /* "TMDfastTG3NL\n" */
extern s32 warn_dmyTMDfastTG3NL;

void *dmyGsTMDfastTG3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTG3NL == 0)
    {
        printf(str_dmyTMDfastTG3NL);
        warn_dmyTMDfastTG3NL = 1;
    }
    return arg3;
}
