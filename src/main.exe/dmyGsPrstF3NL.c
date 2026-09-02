#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyPrstF3NL[]; /* "PrstF3NL\n" */
extern s32 warn_dmyPrstF3NL;

void *dmyGsPrstF3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstF3NL == 0)
    {
        printf(str_dmyPrstF3NL);
        warn_dmyPrstF3NL = 1;
    }
    return arg3;
}
