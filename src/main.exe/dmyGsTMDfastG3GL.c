#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastG3GL (0x800686dc) — LIBGS "dummy" TMD-fast-draw placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastG3GL[]; /* "TMDfastG3GL\n" */
extern s32 warn_dmyTMDfastG3GL;

void *dmyGsTMDfastG3GL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastG3GL == 0)
    {
        printf(str_dmyTMDfastG3GL);
        warn_dmyTMDfastG3GL = 1;
    }
    return arg3;
}
