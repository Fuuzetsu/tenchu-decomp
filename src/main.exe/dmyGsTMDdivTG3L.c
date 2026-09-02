#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivTG3L[]; /* "TMDdivTG3L\n" */
extern s32 warn_dmyTMDdivTG3L;

void *dmyGsTMDdivTG3L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivTG3L == 0)
    {
        printf(str_dmyTMDdivTG3L);
        warn_dmyTMDdivTG3L = 1;
    }
    return arg3;
}
