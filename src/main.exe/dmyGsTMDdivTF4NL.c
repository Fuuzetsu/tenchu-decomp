#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivTF4NL (0x800679a4) — LIBGS "dummy" TMD-subdivision placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivTF4NL[]; /* "TMDdivTF4NL\n" */
extern s32 warn_dmyTMDdivTF4NL;

void *dmyGsTMDdivTF4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivTF4NL == 0)
    {
        printf(str_dmyTMDdivTF4NL);
        warn_dmyTMDdivTF4NL = 1;
    }
    return arg3;
}
