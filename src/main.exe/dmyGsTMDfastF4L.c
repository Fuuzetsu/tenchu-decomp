#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastF4L (0x80068214) — LIBGS "dummy" TMD-fast-draw placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastF4L[]; /* "TMDfastF4L\n" */
extern s32 warn_dmyTMDfastF4L;

void *dmyGsTMDfastF4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastF4L == 0)
    {
        printf(str_dmyTMDfastF4L);
        warn_dmyTMDfastF4L = 1;
    }
    return arg3;
}
