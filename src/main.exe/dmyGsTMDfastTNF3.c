#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastTNF3[]; /* "TMDfastTNF3\n" */
extern s32 warn_dmyTMDfastTNF3;

void *dmyGsTMDfastTNF3(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDfastTNF3 == 0)
    {
        printf(str_dmyTMDfastTNF3);
        warn_dmyTMDfastTNF3 = 1;
    }
    return arg2;
}
