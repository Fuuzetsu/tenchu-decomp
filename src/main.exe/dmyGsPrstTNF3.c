#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyPrstTNF3[]; /* "PrstTNF3\n" */
extern s32 warn_dmyPrstTNF3;

void *dmyGsPrstTNF3(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTNF3 == 0)
    {
        printf(str_dmyPrstTNF3);
        warn_dmyPrstTNF3 = 1;
    }
    return arg3;
}
