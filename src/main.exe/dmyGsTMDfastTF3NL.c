#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastTF3NL (0x80067f44) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastTF3NL[]; /* "TMDfastTF3NL\n" */
extern s32 warn_dmyTMDfastTF3NL;

void *dmyGsTMDfastTF3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTF3NL == 0)
    {
        printf(str_dmyTMDfastTF3NL);
        warn_dmyTMDfastTF3NL = 1;
    }
    return arg3;
}
