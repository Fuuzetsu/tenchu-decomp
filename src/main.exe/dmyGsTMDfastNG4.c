#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastNG4[]; /* "TMDfastNG4\n" */
extern s32 warn_dmyTMDfastNG4;

void *dmyGsTMDfastNG4(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDfastNG4 == 0)
    {
        printf(str_dmyTMDfastNG4);
        warn_dmyTMDfastNG4 = 1;
    }
    return arg2;
}
