#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastG4LFG (0x800682ec) — LIBGS "dummy" TMD-fast-draw placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastG4LFG[]; /* "TMDfastG4LFG\n" */
extern s32 warn_dmyTMDfastG4LFG;

void *dmyGsTMDfastG4LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastG4LFG == 0)
    {
        printf(str_dmyTMDfastG4LFG);
        warn_dmyTMDfastG4LFG = 1;
    }
    return arg3;
}
