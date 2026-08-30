#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastTF4NL (0x800683c4) — LIBGS "dummy" TMD-fast-draw placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastTF4NL[]; /* "TMDfastTF4NL\n" */
extern s32 warn_dmyTMDfastTF4NL;

void *dmyGsTMDfastTF4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTF4NL == 0)
    {
        printf(str_dmyTMDfastTF4NL);
        warn_dmyTMDfastTF4NL = 1;
    }
    return arg3;
}
