#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyPrstG4NL[]; /* "PrstG4NL\n" */
extern s32 warn_dmyPrstG4NL;

void *dmyGsPrstG4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstG4NL == 0)
    {
        printf(str_dmyPrstG4NL);
        warn_dmyPrstG4NL = 1;
    }
    return arg3;
}
