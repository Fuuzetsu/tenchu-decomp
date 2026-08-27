#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastF3LFG (0x80067d4c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastF3LFG[]; /* "TMDfastF3LFG\n" */
extern s32 warn_dmyTMDfastF3LFG;

void *dmyGsTMDfastF3LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastF3LFG == 0)
    {
        printf(str_dmyTMDfastF3LFG);
        warn_dmyTMDfastF3LFG = 1;
    }
    return arg3;
}
