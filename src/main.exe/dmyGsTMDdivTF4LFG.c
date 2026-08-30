#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivTF4LFG (0x800679ec) — LIBGS "dummy" TMD-subdivision placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivTF4LFG[]; /* "TMDdivTF4LFG\n" */
extern s32 warn_dmyTMDdivTF4LFG;

void *dmyGsTMDdivTF4LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivTF4LFG == 0)
    {
        printf(str_dmyTMDdivTF4LFG);
        warn_dmyTMDdivTF4LFG = 1;
    }
    return arg3;
}
