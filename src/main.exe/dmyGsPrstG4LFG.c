#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstG4LFG (0x8006768c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstG4LFG[]; /* "PrstG4LFG\n" */
extern s32 warn_dmyPrstG4LFG;

void *dmyGsPrstG4LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstG4LFG == 0)
    {
        printf(str_dmyPrstG4LFG);
        warn_dmyPrstG4LFG = 1;
    }
    return arg3;
}
