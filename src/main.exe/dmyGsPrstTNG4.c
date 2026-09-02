#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyPrstTNG4[]; /* "PrstTNG4\n" */
extern s32 warn_dmyPrstTNG4;

void *dmyGsPrstTNG4(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTNG4 == 0)
    {
        printf(str_dmyPrstTNG4);
        warn_dmyPrstTNG4 = 1;
    }
    return arg3;
}
