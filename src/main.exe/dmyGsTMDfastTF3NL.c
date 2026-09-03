#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDfastTF3NL[]; /* "TMDfastTF3NL\n" */
extern s32 warn_dmyTMDfastTF3NL;

void *dmyGsTMDfastTF3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastTF3NL == 0)
    {
        printf(str_dmyTMDfastTF3NL);
        warn_dmyTMDfastTF3NL = 1;
    }
    return arg3;
}
