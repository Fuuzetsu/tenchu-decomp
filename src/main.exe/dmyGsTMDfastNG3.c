#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastNG3[]; /* "TMDfastNG3\n" */
extern s32 warn_dmyTMDfastNG3;

void *dmyGsTMDfastNG3(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDfastNG3 == 0)
    {
        printf(str_dmyTMDfastNG3);
        warn_dmyTMDfastNG3 = 1;
    }
    return arg2;
}
