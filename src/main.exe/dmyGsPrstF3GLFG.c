#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstF3GLFG (0x800687fc) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstF3GLFG[]; /* "PrstF3GLFG\n" */
extern s32 warn_dmyPrstF3GLFG;

void *dmyGsPrstF3GLFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstF3GLFG == 0)
    {
        printf(str_dmyPrstF3GLFG);
        warn_dmyPrstF3GLFG = 1;
    }
    return arg3;
}
