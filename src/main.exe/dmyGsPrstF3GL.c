#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyPrstF3GL[]; /* "PrstF3GL\n" */
extern s32 warn_dmyPrstF3GL;

void *dmyGsPrstF3GL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstF3GL == 0)
    {
        printf(str_dmyPrstF3GL);
        warn_dmyPrstF3GL = 1;
    }
    return arg3;
}
