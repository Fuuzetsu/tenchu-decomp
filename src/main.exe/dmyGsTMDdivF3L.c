#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivF3L[]; /* "TMDdivF3L\n" */
extern s32 warn_dmyTMDdivF3L;

void *dmyGsTMDdivF3L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivF3L == 0)
    {
        printf(str_dmyTMDdivF3L);
        warn_dmyTMDdivF3L = 1;
    }
    return arg3;
}
