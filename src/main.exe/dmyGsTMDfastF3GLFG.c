#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastF3GLFG (0x8006864c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastF3GLFG[]; /* "TMDfastF3GLFG\n" */
extern s32 warn_dmyTMDfastF3GLFG;

void *dmyGsTMDfastF3GLFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyTMDfastF3GLFG == 0)
    {
        printf(str_dmyTMDfastF3GLFG);
        warn_dmyTMDfastF3GLFG = 1;
    }
    return arg3;
}
