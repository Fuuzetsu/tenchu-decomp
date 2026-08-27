#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstTG3LFG (0x8006720c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstTG3LFG[]; /* "PrstTG3LFG\n" */
extern s32 warn_dmyPrstTG3LFG;

void *dmyGsPrstTG3LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTG3LFG == 0)
    {
        printf(str_dmyPrstTG3LFG);
        warn_dmyPrstTG3LFG = 1;
    }
    return arg3;
}
