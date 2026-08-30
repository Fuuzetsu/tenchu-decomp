#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastF3GL (0x80068604) — LIBGS "dummy" TMD-fast-draw placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastF3GL[]; /* "TMDfastF3GL\n" */
extern s32 warn_dmyTMDfastF3GL;

void *dmyGsTMDfastF3GL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastF3GL == 0)
    {
        printf(str_dmyTMDfastF3GL);
        warn_dmyTMDfastF3GL = 1;
    }
    return arg3;
}
