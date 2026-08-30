#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivTF4L (0x80067a34) — LIBGS "dummy" TMD-subdivision placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivTF4L[]; /* "TMDdivTF4L\n" */
extern s32 warn_dmyTMDdivTF4L;

void *dmyGsTMDdivTF4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivTF4L == 0)
    {
        printf(str_dmyTMDdivTF4L);
        warn_dmyTMDdivTF4L = 1;
    }
    return arg3;
}
