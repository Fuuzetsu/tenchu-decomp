#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastTG4L (0x80068574) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastTG4L[]; /* "TMDfastTG4L\n" */
extern s32 warn_dmyTMDfastTG4L;

void *dmyGsTMDfastTG4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTG4L == 0)
    {
        printf(str_dmyTMDfastTG4L);
        warn_dmyTMDfastTG4L = 1;
    }
    return arg3;
}
