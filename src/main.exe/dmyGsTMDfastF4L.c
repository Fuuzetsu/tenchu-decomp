#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastF4L[]; /* "TMDfastF4L\n" */
extern s32 warn_dmyTMDfastF4L;

void *dmyGsTMDfastF4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastF4L == 0)
    {
        printf(str_dmyTMDfastF4L);
        warn_dmyTMDfastF4L = 1;
    }
    return arg3;
}
