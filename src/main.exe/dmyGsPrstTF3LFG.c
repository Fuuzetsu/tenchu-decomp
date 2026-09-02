#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyPrstTF3LFG[]; /* "PrstTF3LFG\n" */
extern s32 warn_dmyPrstTF3LFG;

void *dmyGsPrstTF3LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTF3LFG == 0)
    {
        printf(str_dmyPrstTF3LFG);
        warn_dmyPrstTF3LFG = 1;
    }
    return arg3;
}
