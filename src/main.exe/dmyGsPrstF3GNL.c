#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstF3GNL (0x80068844) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstF3GNL[]; /* "PrstF3GNL\n" */
extern s32 warn_dmyPrstF3GNL;

void *dmyGsPrstF3GNL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstF3GNL == 0)
    {
        printf(str_dmyPrstF3GNL);
        warn_dmyPrstF3GNL = 1;
    }
    return arg3;
}
