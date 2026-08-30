#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastF4LFG (0x800681cc) — LIBGS "dummy" TMD-fast-draw placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastF4LFG[]; /* "TMDfastF4LFG\n" */
extern s32 warn_dmyTMDfastF4LFG;

void *dmyGsTMDfastF4LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastF4LFG == 0)
    {
        printf(str_dmyTMDfastF4LFG);
        warn_dmyTMDfastF4LFG = 1;
    }
    return arg3;
}
