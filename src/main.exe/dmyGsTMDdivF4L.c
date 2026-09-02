#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivF4L[]; /* "TMDdivF4L\n" */
extern s32 warn_dmyTMDdivF4L;

void *dmyGsTMDdivF4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivF4L == 0)
    {
        printf(str_dmyTMDdivF4L);
        warn_dmyTMDdivF4L = 1;
    }
    return arg3;
}
