#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyPrstG3L[]; /* "PrstG3L\n" */
extern s32 warn_dmyPrstG3L;

void *dmyGsPrstG3L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstG3L == 0)
    {
        printf(str_dmyPrstG3L);
        warn_dmyPrstG3L = 1;
    }
    return arg3;
}
