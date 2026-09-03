#include "common.h"
#include "main.exe.h"

extern char str_dmyPrstTF3NL[]; /* "PrstTF3NL\n" */
extern s32 warn_dmyPrstTF3NL;

void *dmyGsPrstTF3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTF3NL == 0)
    {
        printf(str_dmyPrstTF3NL);
        warn_dmyPrstTF3NL = 1;
    }
    return arg3;
}
