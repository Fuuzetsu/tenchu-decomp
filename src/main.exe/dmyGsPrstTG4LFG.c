#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyPrstTG4LFG[]; /* "PrstTG4LFG\n" */
extern s32 warn_dmyPrstTG4LFG;

void *dmyGsPrstTG4LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTG4LFG == 0)
    {
        printf(str_dmyPrstTG4LFG);
        warn_dmyPrstTG4LFG = 1;
    }
    return arg3;
}
