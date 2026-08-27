#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstTF4NL (0x80067884) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstTF4NL[]; /* "PrstTF4NL\n" */
extern s32 warn_dmyPrstTF4NL;

void *dmyGsPrstTF4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTF4NL == 0)
    {
        printf(str_dmyPrstTF4NL);
        warn_dmyPrstTF4NL = 1;
    }
    return arg3;
}
