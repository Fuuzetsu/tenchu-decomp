#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstTNG4 (0x80067b9c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstTNG4[]; /* "PrstTNG4\n" */
extern s32 warn_dmyPrstTNG4;

void *dmyGsPrstTNG4(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTNG4 == 0)
    {
        printf(str_dmyPrstTNG4);
        warn_dmyPrstTNG4 = 1;
    }
    return arg3;
}
