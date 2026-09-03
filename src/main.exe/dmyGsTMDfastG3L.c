#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastG3L[]; /* "TMDfastG3L\n" */
extern s32 warn_dmyTMDfastG3L;

void *dmyGsTMDfastG3L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastG3L == 0)
    {
        printf(str_dmyTMDfastG3L);
        warn_dmyTMDfastG3L = 1;
    }
    return arg3;
}
