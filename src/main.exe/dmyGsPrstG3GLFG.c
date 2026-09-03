#include "common.h"
#include "main.exe.h"

extern char str_dmyPrstG3GLFG[]; /* "PrstG3GLFG\n" */
extern s32 warn_dmyPrstG3GLFG;

void *dmyGsPrstG3GLFG(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstG3GLFG == 0)
    {
        printf(str_dmyPrstG3GLFG);
        warn_dmyPrstG3GLFG = 1;
    }
    return arg3;
}
