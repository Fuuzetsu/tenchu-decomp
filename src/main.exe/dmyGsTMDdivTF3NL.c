#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivTF3NL (0x800670a4) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivTF3NL[]; /* "TMDdivTF3NL\n" */
extern s32 warn_dmyTMDdivTF3NL;

void *dmyGsTMDdivTF3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivTF3NL == 0)
    {
        printf(str_dmyTMDdivTF3NL);
        warn_dmyTMDdivTF3NL = 1;
    }
    return arg3;
}
