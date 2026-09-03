#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastTG4L[]; /* "TMDfastTG4L\n" */
extern s32 warn_dmyTMDfastTG4L;

void *dmyGsTMDfastTG4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTG4L == 0)
    {
        printf(str_dmyTMDfastTG4L);
        warn_dmyTMDfastTG4L = 1;
    }
    return arg3;
}
