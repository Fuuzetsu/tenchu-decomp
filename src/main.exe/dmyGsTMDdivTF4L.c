#include "common.h"
#include "main.exe.h"

extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivTF4L[]; /* "TMDdivTF4L\n" */
extern s32 warn_dmyTMDdivTF4L;

void *dmyGsTMDdivTF4L(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivTF4L == 0)
    {
        printf(str_dmyTMDdivTF4L);
        warn_dmyTMDdivTF4L = 1;
    }
    return arg3;
}
