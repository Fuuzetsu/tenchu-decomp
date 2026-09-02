#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyPrstF3GNL[]; /* "PrstF3GNL\n" */
extern s32 warn_dmyPrstF3GNL;

void *dmyGsPrstF3GNL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstF3GNL == 0)
    {
        printf(str_dmyPrstF3GNL);
        warn_dmyPrstF3GNL = 1;
    }
    return arg3;
}
