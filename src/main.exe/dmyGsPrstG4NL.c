#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstG4NL (0x80067644) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstG4NL[]; /* "PrstG4NL\n" */
extern s32 warn_dmyPrstG4NL;

void *dmyGsPrstG4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstG4NL == 0)
    {
        printf(str_dmyPrstG4NL);
        warn_dmyPrstG4NL = 1;
    }
    return arg3;
}
