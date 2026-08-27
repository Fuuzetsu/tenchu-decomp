#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstG3LFG (0x80066d8c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstG3LFG[]; /* "PrstG3LFG\n" */
extern s32 warn_dmyPrstG3LFG;

void *dmyGsPrstG3LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstG3LFG == 0)
    {
        printf(str_dmyPrstG3LFG);
        warn_dmyPrstG3LFG = 1;
    }
    return arg3;
}
