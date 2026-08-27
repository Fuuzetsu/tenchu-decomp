#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstTG3L (0x80067254) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstTG3L[]; /* "PrstTG3L\n" */
extern s32 warn_dmyPrstTG3L;

void *dmyGsPrstTG3L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTG3L == 0)
    {
        printf(str_dmyPrstTG3L);
        warn_dmyPrstTG3L = 1;
    }
    return arg3;
}
