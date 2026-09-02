#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastTNG4[]; /* "TMDfastTNG4\n" */
extern s32 warn_dmyTMDfastTNG4;

void *dmyGsTMDfastTNG4(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDfastTNG4 == 0)
    {
        printf(str_dmyTMDfastTNG4);
        warn_dmyTMDfastTNG4 = 1;
    }
    return arg2;
}
