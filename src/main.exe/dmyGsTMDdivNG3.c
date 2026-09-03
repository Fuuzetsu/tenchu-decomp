#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDdivNG3[]; /* "TMDdivNG3\n" */
extern s32 warn_dmyTMDdivNG3;

void *dmyGsTMDdivNG3(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDdivNG3 == 0)
    {
        printf(str_dmyTMDdivNG3);
        warn_dmyTMDdivNG3 = 1;
    }
    return arg2;
}
