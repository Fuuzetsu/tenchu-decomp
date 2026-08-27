#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstF3LFG (0x80066b4c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstF3LFG[]; /* "PrstF3LFG\n" */
extern s32 warn_dmyPrstF3LFG;

void *dmyGsPrstF3LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstF3LFG == 0)
    {
        printf(str_dmyPrstF3LFG);
        warn_dmyPrstF3LFG = 1;
    }
    return arg3;
}
