#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastG3LFG (0x80067e6c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastG3LFG[]; /* "TMDfastG3LFG\n" */
extern s32 warn_dmyTMDfastG3LFG;

void *dmyGsTMDfastG3LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastG3LFG == 0)
    {
        printf(str_dmyTMDfastG3LFG);
        warn_dmyTMDfastG3LFG = 1;
    }
    return arg3;
}
