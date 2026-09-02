#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivTF3L[]; /* "TMDdivTF3L\n" */
extern s32 warn_dmyTMDdivTF3L;

void *dmyGsTMDdivTF3L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivTF3L == 0)
    {
        printf(str_dmyTMDdivTF3L);
        warn_dmyTMDdivTF3L = 1;
    }
    return arg3;
}
