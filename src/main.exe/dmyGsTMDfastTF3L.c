#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastTF3L (0x80067fd4) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastTF3L[]; /* "TMDfastTF3L\n" */
extern s32 warn_dmyTMDfastTF3L;

void *dmyGsTMDfastTF3L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTF3L == 0)
    {
        printf(str_dmyTMDfastTF3L);
        warn_dmyTMDfastTF3L = 1;
    }
    return arg3;
}
