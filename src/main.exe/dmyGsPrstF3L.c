#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstF3L (0x80066b94) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstF3L[]; /* "PrstF3L\n" */
extern s32 warn_dmyPrstF3L;

void *dmyGsPrstF3L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstF3L == 0)
    {
        printf(str_dmyPrstF3L);
        warn_dmyPrstF3L = 1;
    }
    return arg3;
}
