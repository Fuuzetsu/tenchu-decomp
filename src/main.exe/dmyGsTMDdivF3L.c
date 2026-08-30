#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivF3L (0x80066cb4) — LIBGS "dummy" TMD-subdivision placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivF3L[]; /* "TMDdivF3L\n" */
extern s32 warn_dmyTMDdivF3L;

void *dmyGsTMDdivF3L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivF3L == 0)
    {
        printf(str_dmyTMDdivF3L);
        warn_dmyTMDdivF3L = 1;
    }
    return arg3;
}
