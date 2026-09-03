#include "common.h"
#include "main.exe.h"

extern char str_dmyPrstTG4NL[]; /* "PrstTG4NL\n" */
extern s32 warn_dmyPrstTG4NL;

void *dmyGsPrstTG4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTG4NL == 0)
    {
        printf(str_dmyPrstTG4NL);
        warn_dmyPrstTG4NL = 1;
    }
    return arg3;
}
