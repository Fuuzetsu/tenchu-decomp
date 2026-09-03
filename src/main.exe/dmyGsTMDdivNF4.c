#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDdivNF4[]; /* "TMDdivNF4\n" */
extern s32 warn_dmyTMDdivNF4;

void *dmyGsTMDdivNF4(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDdivNF4 == 0)
    {
        printf(str_dmyTMDdivNF4);
        warn_dmyTMDdivNF4 = 1;
    }
    return arg2;
}
