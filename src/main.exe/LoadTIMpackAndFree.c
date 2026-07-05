#include "common.h"
#include "main.exe.h"

/*
 * LoadTIMpackAndFree (0x800188d8) — identical shape to LoadTIMAndFree just
 * above it: loads a TIM-pack via LoadTIMpack, then frees the buffer via
 * vfree. `tim` is cached in a callee-saved reg across both calls (a plain
 * parameter read twice needs no separate temp — cookbook's cached-pointer
 * rule).
 */
extern void LoadTIMpack(u_long *tim);
extern void vfree(void *p);

void LoadTIMpackAndFree(u_long *tim)
{
    LoadTIMpack(tim);
    vfree(tim);
}
