#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstTG3NL (0x800671c4) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstTG3NL[]; /* "PrstTG3NL\n" */
extern s32 warn_dmyPrstTG3NL;

void *dmyGsPrstTG3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTG3NL == 0)
    {
        printf(str_dmyPrstTG3NL);
        warn_dmyPrstTG3NL = 1;
    }
    return arg3;
}
