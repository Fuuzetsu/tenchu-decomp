#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDdivTNF3[]; /* "TMDdivTNF3\n" */
extern s32 warn_dmyTMDdivTNF3;

void *dmyGsTMDdivTNF3(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivTNF3 == 0)
    {
        printf(str_dmyTMDdivTNF3);
        warn_dmyTMDdivTNF3 = 1;
    }
    return arg3;
}
