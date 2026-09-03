#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastTF4NL[]; /* "TMDfastTF4NL\n" */
extern s32 warn_dmyTMDfastTF4NL;

void *dmyGsTMDfastTF4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTF4NL == 0)
    {
        printf(str_dmyTMDfastTF4NL);
        warn_dmyTMDfastTF4NL = 1;
    }
    return arg3;
}
