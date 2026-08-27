#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstTG4NL (0x80067ac4) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstTG4NL[]; /* "PrstTG4NL\n" */
extern s32 warn_dmyPrstTG4NL;

void *dmyGsPrstTG4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTG4NL == 0)
    {
        printf(str_dmyPrstTG4NL);
        warn_dmyPrstTG4NL = 1;
    }
    return arg3;
}
