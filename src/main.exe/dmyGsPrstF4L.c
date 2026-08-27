#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstF4L (0x80067494) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstF4L[]; /* "PrstF4L\n" */
extern s32 warn_dmyPrstF4L;

void *dmyGsPrstF4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstF4L == 0)
    {
        printf(str_dmyPrstF4L);
        warn_dmyPrstF4L = 1;
    }
    return arg3;
}
