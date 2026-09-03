#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastG4L[]; /* "TMDfastG4L\n" */
extern s32 warn_dmyTMDfastG4L;

void *dmyGsTMDfastG4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastG4L == 0)
    {
        printf(str_dmyTMDfastG4L);
        warn_dmyTMDfastG4L = 1;
    }
    return arg3;
}
