#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastTG3L (0x800680f4) — LIBGS "dummy" TMD-fast-draw placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastTG3L[]; /* "TMDfastTG3L\n" */
extern s32 warn_dmyTMDfastTG3L;

void *dmyGsTMDfastTG3L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTG3L == 0)
    {
        printf(str_dmyTMDfastTG3L);
        warn_dmyTMDfastTG3L = 1;
    }
    return arg3;
}
