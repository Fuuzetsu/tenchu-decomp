#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastF3GL[]; /* "TMDfastF3GL\n" */
extern s32 warn_dmyTMDfastF3GL;

void *dmyGsTMDfastF3GL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastF3GL == 0)
    {
        printf(str_dmyTMDfastF3GL);
        warn_dmyTMDfastF3GL = 1;
    }
    return arg3;
}
