#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivTNG3[]; /* "TMDdivTNG3\n" */
extern s32 warn_dmyTMDdivTNG3;

void *dmyGsTMDdivTNG3(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDdivTNG3 == 0)
    {
        printf(str_dmyTMDdivTNG3);
        warn_dmyTMDdivTNG3 = 1;
    }
    return arg2;
}
