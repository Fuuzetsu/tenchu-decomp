#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstF3GL (0x800687b4) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstF3GL[]; /* "PrstF3GL\n" */
extern s32 warn_dmyPrstF3GL;

void *dmyGsPrstF3GL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstF3GL == 0)
    {
        printf(str_dmyPrstF3GL);
        warn_dmyPrstF3GL = 1;
    }
    return arg3;
}
