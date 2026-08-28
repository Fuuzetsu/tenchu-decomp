#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/*
 * DoBriefingAndInventorySelection (0x800164e0, 0x8c bytes) — snapshot the
 * briefing screen's VRAM rect into a freshly valloc'd buffer via
 * StoreImage2, run the briefing/inventory screen, then restore the VRAM
 * via LoadImage2 and free the buffer.
 *
 * The rect is addressed here as `_gp` only because the MIPS gp anchor
 * happens to land exactly on this RECT object in sdata (0x80097698), and
 * that reserved linker symbol is the one name the address already has.
 * The original surely called it something like BriefRect; giving it its
 * own symbol needs the normal-link lane to treat it as section-owned
 * data rather than a pinned numeric alias of the gp anchor — parked in
 * PLAN.md until the shiftability contract can express that.
 *
 * `r = _gp;` is the align-2 RECT struct-copy idiom (lwl/lwr +
 * swl/swr pairs, see UpdateOrnament.c's SVECTOR copy) — m2c mis-decodes the
 * two unaligned-word loads as extra `valloc` arguments (a leftover register
 * still holding the copy's second word); Ghidra's single-argument
 * `valloc(0x20000)` is the real call (m2c over-count rule).
 */
extern RECT _gp[];

extern void *valloc(u32 size);
extern void vfree(void *p);
extern void ResetInventory(void);
extern void BriefingAndInventorySelectionScreen(void);

void DoBriefingAndInventorySelection(void)
{
    RECT r;
    u_long *p;

    r = _gp[0];
    p = (u_long *)valloc(0x20000);
    StoreImage2(&r, p);
    ResetInventory();
    BriefingAndInventorySelectionScreen();
    LoadImage2(&r, p);
    vfree(p);
}
