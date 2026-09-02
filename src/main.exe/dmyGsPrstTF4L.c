#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyPrstTF4L[]; /* "PrstTF4L\n" */
extern s32 warn_dmyPrstTF4L;

void *dmyGsPrstTF4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTF4L == 0)
    {
        printf(str_dmyPrstTF4L);
        warn_dmyPrstTF4L = 1;
    }
    return arg3;
}
