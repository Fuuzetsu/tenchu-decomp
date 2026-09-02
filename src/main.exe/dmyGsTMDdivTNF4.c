#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivTNF4[]; /* "TMDdivTNF4\n" */
extern s32 warn_dmyTMDdivTNF4;

void *dmyGsTMDdivTNF4(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDdivTNF4 == 0)
    {
        printf(str_dmyTMDdivTNF4);
        warn_dmyTMDdivTNF4 = 1;
    }
    return arg2;
}
