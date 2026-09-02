#include "common.h"
#include "main.exe.h"

extern void vfree(void *p);

void LoadTIMAndFree(u_long *tim)
{
    LoadTIM(tim);
    vfree(tim);
}
