#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstTNG3 (0x8006729c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstTNG3[]; /* "PrstTNG3\n" */
extern s32 warn_dmyPrstTNG3;

void *dmyGsPrstTNG3(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTNG3 == 0)
    {
        printf(str_dmyPrstTNG3);
        warn_dmyPrstTNG3 = 1;
    }
    return arg3;
}
