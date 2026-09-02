#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyPrstNG4[]; /* "PrstNG4\n" */
extern s32 warn_dmyPrstNG4;

void *dmyGsPrstNG4(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstNG4 == 0)
    {
        printf(str_dmyPrstNG4);
        warn_dmyPrstNG4 = 1;
    }
    return arg3;
}
