#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivF3LFG (0x80066c6c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivF3LFG[]; /* "TMDdivF3LFG\n" */
extern s32 warn_dmyTMDdivF3LFG;

void *dmyGsTMDdivF3LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivF3LFG == 0)
    {
        printf(str_dmyTMDdivF3LFG);
        warn_dmyTMDdivF3LFG = 1;
    }
    return arg3;
}
