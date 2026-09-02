#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyPrstTG4L[]; /* "PrstTG4L\n" */
extern s32 warn_dmyPrstTG4L;

void *dmyGsPrstTG4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTG4L == 0)
    {
        printf(str_dmyPrstTG4L);
        warn_dmyPrstTG4L = 1;
    }
    return arg3;
}
