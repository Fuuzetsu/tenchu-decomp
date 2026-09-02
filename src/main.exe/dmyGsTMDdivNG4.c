#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivNG4[]; /* "TMDdivNG4\n" */
extern s32 warn_dmyTMDdivNG4;

void *dmyGsTMDdivNG4(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDdivNG4 == 0)
    {
        printf(str_dmyTMDdivNG4);
        warn_dmyTMDdivNG4 = 1;
    }
    return arg2;
}
