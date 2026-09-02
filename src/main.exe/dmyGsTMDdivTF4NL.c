#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivTF4NL[]; /* "TMDdivTF4NL\n" */
extern s32 warn_dmyTMDdivTF4NL;

void *dmyGsTMDdivTF4NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivTF4NL == 0)
    {
        printf(str_dmyTMDdivTF4NL);
        warn_dmyTMDdivTF4NL = 1;
    }
    return arg3;
}
