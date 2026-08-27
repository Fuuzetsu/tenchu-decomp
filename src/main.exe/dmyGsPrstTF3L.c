#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstTF3L (0x80067014) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstTF3L[]; /* "PrstTF3L\n" */
extern s32 warn_dmyPrstTF3L;

void *dmyGsPrstTF3L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTF3L == 0)
    {
        printf(str_dmyPrstTF3L);
        warn_dmyPrstTF3L = 1;
    }
    return arg3;
}
