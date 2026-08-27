#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstF4LFG (0x8006744c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstF4LFG[]; /* "PrstF4LFG\n" */
extern s32 warn_dmyPrstF4LFG;

void *dmyGsPrstF4LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstF4LFG == 0)
    {
        printf(str_dmyPrstF4LFG);
        warn_dmyPrstF4LFG = 1;
    }
    return arg3;
}
