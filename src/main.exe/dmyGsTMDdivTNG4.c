#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDdivTNG4[]; /* "TMDdivTNG4\n" */
extern s32 warn_dmyTMDdivTNG4;

void *dmyGsTMDdivTNG4(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDdivTNG4 == 0)
    {
        printf(str_dmyTMDdivTNG4);
        warn_dmyTMDdivTNG4 = 1;
    }
    return arg2;
}
