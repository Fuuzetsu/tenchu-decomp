#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstG3GL (0x8006888c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstG3GL[]; /* "PrstG3GL\n" */
extern s32 warn_dmyPrstG3GL;

void *dmyGsPrstG3GL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstG3GL == 0)
    {
        printf(str_dmyPrstG3GL);
        warn_dmyPrstG3GL = 1;
    }
    return arg3;
}
