#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivG4L (0x800677f4) — LIBGS "dummy" TMD-subdivision placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivG4L[]; /* "TMDdivG4L\n" */
extern s32 warn_dmyTMDdivG4L;

void *dmyGsTMDdivG4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivG4L == 0)
    {
        printf(str_dmyTMDdivG4L);
        warn_dmyTMDdivG4L = 1;
    }
    return arg3;
}
