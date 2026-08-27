#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastTG4LFG (0x8006852c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastTG4LFG[]; /* "TMDfastTG4LFG\n" */
extern s32 warn_dmyTMDfastTG4LFG;

void *dmyGsTMDfastTG4LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTG4LFG == 0)
    {
        printf(str_dmyTMDfastTG4LFG);
        warn_dmyTMDfastTG4LFG = 1;
    }
    return arg3;
}
