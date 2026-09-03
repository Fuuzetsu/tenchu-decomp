#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastTNF4[]; /* "TMDfastTNF4\n" */
extern s32 warn_dmyTMDfastTNF4;

void *dmyGsTMDfastTNF4(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDfastTNF4 == 0)
    {
        printf(str_dmyTMDfastTNF4);
        warn_dmyTMDfastTNF4 = 1;
    }
    return arg2;
}
