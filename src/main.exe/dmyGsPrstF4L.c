#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyPrstF4L[]; /* "PrstF4L\n" */
extern s32 warn_dmyPrstF4L;

void *dmyGsPrstF4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstF4L == 0)
    {
        printf(str_dmyPrstF4L);
        warn_dmyPrstF4L = 1;
    }
    return arg3;
}
