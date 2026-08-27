#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivF4L (0x800675b4) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivF4L[]; /* "TMDdivF4L\n" */
extern s32 warn_dmyTMDdivF4L;

void *dmyGsTMDdivF4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivF4L == 0)
    {
        printf(str_dmyTMDdivF4L);
        warn_dmyTMDdivF4L = 1;
    }
    return arg3;
}
