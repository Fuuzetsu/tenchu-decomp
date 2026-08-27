#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstG3NL (0x80066d44) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstG3NL[]; /* "PrstG3NL\n" */
extern s32 warn_dmyPrstG3NL;

void *dmyGsPrstG3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstG3NL == 0)
    {
        printf(str_dmyPrstG3NL);
        warn_dmyPrstG3NL = 1;
    }
    return arg3;
}
