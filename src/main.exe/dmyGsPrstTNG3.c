#include "common.h"
#include "main.exe.h"

extern char str_dmyPrstTNG3[]; /* "PrstTNG3\n" */
extern s32 warn_dmyPrstTNG3;

void *dmyGsPrstTNG3(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstTNG3 == 0)
    {
        printf(str_dmyPrstTNG3);
        warn_dmyPrstTNG3 = 1;
    }
    return arg3;
}
