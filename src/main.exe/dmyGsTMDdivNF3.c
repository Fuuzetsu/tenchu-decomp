#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivNF3[]; /* "TMDdivNF3\n" */
extern s32 warn_dmyTMDdivNF3;

void *dmyGsTMDdivNF3(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDdivNF3 == 0)
    {
        printf(str_dmyTMDdivNF3);
        warn_dmyTMDdivNF3 = 1;
    }
    return arg2;
}
