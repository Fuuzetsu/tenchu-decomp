#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastF3L[]; /* "TMDfastF3L\n" */
extern s32 warn_dmyTMDfastF3L;

void *dmyGsTMDfastF3L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastF3L == 0)
    {
        printf(str_dmyTMDfastF3L);
        warn_dmyTMDfastF3L = 1;
    }
    return arg3;
}
