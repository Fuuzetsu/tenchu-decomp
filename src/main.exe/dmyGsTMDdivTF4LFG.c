#include "common.h"
#include "main.exe.h"

extern char str_dmyTMDdivTF4LFG[]; /* "TMDdivTF4LFG\n" */
extern s32 warn_dmyTMDdivTF4LFG;

void *dmyGsTMDdivTF4LFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDdivTF4LFG == 0)
    {
        printf(str_dmyTMDdivTF4LFG);
        warn_dmyTMDdivTF4LFG = 1;
    }
    return arg3;
}
