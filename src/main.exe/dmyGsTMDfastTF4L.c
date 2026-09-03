#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastTF4L[]; /* "TMDfastTF4L\n" */
extern s32 warn_dmyTMDfastTF4L;

void *dmyGsTMDfastTF4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTF4L == 0)
    {
        printf(str_dmyTMDfastTF4L);
        warn_dmyTMDfastTF4L = 1;
    }
    return arg3;
}
