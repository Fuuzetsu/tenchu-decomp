#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastG3NL (0x80067e24) — LIBGS "dummy" TMD-fast-draw placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastG3NL[]; /* "TMDfastG3NL\n" */
extern s32 warn_dmyTMDfastG3NL;

void *dmyGsTMDfastG3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastG3NL == 0)
    {
        printf(str_dmyTMDfastG3NL);
        warn_dmyTMDfastG3NL = 1;
    }
    return arg3;
}
