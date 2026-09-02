#include "common.h"
#include "main.exe.h"

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
