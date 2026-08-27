#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstNG3 (0x80066e1c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstNG3[]; /* "PrstNG3\n" */
extern s32 warn_dmyPrstNG3;

void *dmyGsPrstNG3(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstNG3 == 0)
    {
        printf(str_dmyPrstNG3);
        warn_dmyPrstNG3 = 1;
    }
    return arg3;
}
