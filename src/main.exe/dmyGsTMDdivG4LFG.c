#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivG4LFG (0x800677ac) — LIBGS "dummy" TMD-subdivision placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivG4LFG[]; /* "TMDdivG4LFG\n" */
extern s32 warn_dmyTMDdivG4LFG;

void *dmyGsTMDdivG4LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivG4LFG == 0)
    {
        printf(str_dmyTMDdivG4LFG);
        warn_dmyTMDdivG4LFG = 1;
    }
    return arg3;
}
