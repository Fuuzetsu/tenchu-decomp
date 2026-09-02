#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastNF3[]; /* "TMDfastNF3\n" */
extern s32 warn_dmyTMDfastNF3;

void *dmyGsTMDfastNF3(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDfastNF3 == 0)
    {
        printf(str_dmyTMDfastNF3);
        warn_dmyTMDfastNF3 = 1;
    }
    return arg2;
}
