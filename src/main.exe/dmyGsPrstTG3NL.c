#include "common.h"
#include "main.exe.h"

extern char str_dmyPrstTG3NL[]; /* "PrstTG3NL\n" */
extern s32 warn_dmyPrstTG3NL;

void *dmyGsPrstTG3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTG3NL == 0)
    {
        printf(str_dmyPrstTG3NL);
        warn_dmyPrstTG3NL = 1;
    }
    return arg3;
}
