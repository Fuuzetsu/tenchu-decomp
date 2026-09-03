#include "common.h"
#include "main.exe.h"

extern char str_dmyPrstF4NL[]; /* "PrstF4NL\n" */
extern s32 warn_dmyPrstF4NL;

void *dmyGsPrstF4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstF4NL == 0)
    {
        printf(str_dmyPrstF4NL);
        warn_dmyPrstF4NL = 1;
    }
    return arg3;
}
