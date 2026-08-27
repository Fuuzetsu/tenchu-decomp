#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstTF3NL (0x80066f84) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstTF3NL[]; /* "PrstTF3NL\n" */
extern s32 warn_dmyPrstTF3NL;

void *dmyGsPrstTF3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTF3NL == 0)
    {
        printf(str_dmyPrstTF3NL);
        warn_dmyPrstTF3NL = 1;
    }
    return arg3;
}
