#include "common.h"
#include "main.exe.h"

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
