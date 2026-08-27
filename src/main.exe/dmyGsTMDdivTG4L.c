#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivTG4L (0x80067c74) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivTG4L[]; /* "TMDdivTG4L\n" */
extern s32 warn_dmyTMDdivTG4L;

void *dmyGsTMDdivTG4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivTG4L == 0)
    {
        printf(str_dmyTMDdivTG4L);
        warn_dmyTMDdivTG4L = 1;
    }
    return arg3;
}
