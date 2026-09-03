#include "common.h"
#include "main.exe.h"

extern char str_dmyPrstG3GNL[]; /* "PrstG3GNL\n" */
extern s32 warn_dmyPrstG3GNL;

void *dmyGsPrstG3GNL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstG3GNL == 0)
    {
        printf(str_dmyPrstG3GNL);
        warn_dmyPrstG3GNL = 1;
    }
    return arg3;
}
