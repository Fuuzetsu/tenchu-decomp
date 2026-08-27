#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivTG3LFG (0x8006732c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivTG3LFG[]; /* "TMDdivTG3LFG\n" */
extern s32 warn_dmyTMDdivTG3LFG;

void *dmyGsTMDdivTG3LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivTG3LFG == 0)
    {
        printf(str_dmyTMDdivTG3LFG);
        warn_dmyTMDdivTG3LFG = 1;
    }
    return arg3;
}
