#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstG3L (0x80066dd4) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstG3L[]; /* "PrstG3L\n" */
extern s32 warn_dmyPrstG3L;

void *dmyGsPrstG3L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstG3L == 0)
    {
        printf(str_dmyPrstG3L);
        warn_dmyPrstG3L = 1;
    }
    return arg3;
}
