#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyPrstG3NL[]; /* "PrstG3NL\n" */
extern s32 warn_dmyPrstG3NL;

void *dmyGsPrstG3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstG3NL == 0)
    {
        printf(str_dmyPrstG3NL);
        warn_dmyPrstG3NL = 1;
    }
    return arg3;
}
