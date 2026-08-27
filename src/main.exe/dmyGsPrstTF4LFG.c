#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstTF4LFG (0x800678cc) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstTF4LFG[]; /* "PrstTF4LFG\n" */
extern s32 warn_dmyPrstTF4LFG;

void *dmyGsPrstTF4LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTF4LFG == 0)
    {
        printf(str_dmyPrstTF4LFG);
        warn_dmyPrstTF4LFG = 1;
    }
    return arg3;
}
