#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyPrstNF3[]; /* "PrstNF3\n" */
extern s32 warn_dmyPrstNF3;

void *dmyGsPrstNF3(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstNF3 == 0)
    {
        printf(str_dmyPrstNF3);
        warn_dmyPrstNF3 = 1;
    }
    return arg3;
}
