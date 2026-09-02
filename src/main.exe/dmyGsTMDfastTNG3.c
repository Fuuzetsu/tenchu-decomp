#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastTNG3[]; /* "TMDfastTNG3\n" */
extern s32 warn_dmyTMDfastTNG3;

void *dmyGsTMDfastTNG3(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDfastTNG3 == 0)
    {
        printf(str_dmyTMDfastTNG3);
        warn_dmyTMDfastTNG3 = 1;
    }
    return arg2;
}
