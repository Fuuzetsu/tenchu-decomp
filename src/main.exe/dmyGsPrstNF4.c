#include "common.h"
#include "main.exe.h"

extern char str_dmyPrstNF4[]; /* "PrstNF4\n" */
extern s32 warn_dmyPrstNF4;

void *dmyGsPrstNF4(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstNF4 == 0)
    {
        printf(str_dmyPrstNF4);
        warn_dmyPrstNF4 = 1;
    }
    return arg3;
}
