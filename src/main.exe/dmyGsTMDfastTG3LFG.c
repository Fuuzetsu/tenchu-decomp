#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastTG3LFG (0x800680ac) — LIBGS "dummy" TMD-fast-draw placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastTG3LFG[]; /* "TMDfastTG3LFG\n" */
extern s32 warn_dmyTMDfastTG3LFG;

void *dmyGsTMDfastTG3LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTG3LFG == 0)
    {
        printf(str_dmyTMDfastTG3LFG);
        warn_dmyTMDfastTG3LFG = 1;
    }
    return arg3;
}
