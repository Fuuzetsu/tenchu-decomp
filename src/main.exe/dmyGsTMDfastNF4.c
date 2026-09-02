#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastNF4[]; /* "TMDfastNF4\n" */
extern s32 warn_dmyTMDfastNF4;

void *dmyGsTMDfastNF4(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDfastNF4 == 0)
    {
        printf(str_dmyTMDfastNF4);
        warn_dmyTMDfastNF4 = 1;
    }
    return arg2;
}
