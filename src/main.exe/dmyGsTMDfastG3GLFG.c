#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastG3GLFG (0x80068724) — LIBGS "dummy" TMD-fast-draw placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastG3GLFG[]; /* "TMDfastG3GLFG\n" */
extern s32 warn_dmyTMDfastG3GLFG;

void *dmyGsTMDfastG3GLFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastG3GLFG == 0)
    {
        printf(str_dmyTMDfastG3GLFG);
        warn_dmyTMDfastG3GLFG = 1;
    }
    return arg3;
}
