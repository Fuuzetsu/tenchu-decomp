#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivG3L (0x80066ef4) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivG3L[]; /* "TMDdivG3L\n" */
extern s32 warn_dmyTMDdivG3L;

void *dmyGsTMDdivG3L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivG3L == 0)
    {
        printf(str_dmyTMDdivG3L);
        warn_dmyTMDdivG3L = 1;
    }
    return arg3;
}
