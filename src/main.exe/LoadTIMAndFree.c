#include "common.h"
#include "main.exe.h"

/*
 * LoadTIMAndFree (0x800188ac) — loads a TIM (texture) via LoadTIM, then frees
 * the buffer via vfree. `tim` is cached in a callee-saved reg across both
 * calls (a plain parameter read twice needs no separate temp — see the
 * cookbook's cached-pointer rule, same shape as DrawXG4). Mirrors
 * LoadTIMpackAndFree right below it (LoadTIMpack instead of LoadTIM).
 */
extern void LoadTIM(unsigned long *adr);
extern void vfree(void *p);

void LoadTIMAndFree(u_long *tim)
{
    LoadTIM(tim);
    vfree(tim);
}
