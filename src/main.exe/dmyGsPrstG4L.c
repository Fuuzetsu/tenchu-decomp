#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyPrstG4L[]; /* "PrstG4L\n" */
extern s32 warn_dmyPrstG4L;

void *dmyGsPrstG4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstG4L == 0)
    {
        printf(str_dmyPrstG4L);
        warn_dmyPrstG4L = 1;
    }
    return arg3;
}
