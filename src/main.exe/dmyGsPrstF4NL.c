#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstF4NL (0x80067404) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstF4NL[]; /* "PrstF4NL\n" */
extern s32 warn_dmyPrstF4NL;

void *dmyGsPrstF4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstF4NL == 0)
    {
        printf(str_dmyPrstF4NL);
        warn_dmyPrstF4NL = 1;
    }
    return arg3;
}
