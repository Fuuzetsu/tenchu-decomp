#include "common.h"
#include "main.exe.h"

/*
 * DoBriefingAndInventorySelection (0x800164e0, 0x8c bytes) — snapshot the
 * briefing screen's VRAM rect (_gp, a fixed RECT — same per-TU
 * convention as DemoPatchInit.c/FUN_80038ce0.c, no libgpu.h vendored here)
 * into a freshly valloc'd buffer via StoreImage2, run the briefing/inventory
 * screen, then restore the VRAM via LoadImage2 and free the buffer.
 *
 * `r = _gp;` is the align-2 RECT struct-copy idiom (lwl/lwr +
 * swl/swr pairs, see UpdateOrnament.c's SVECTOR copy) — m2c mis-decodes the
 * two unaligned-word loads as extra `valloc` arguments (a leftover register
 * still holding the copy's second word); Ghidra's single-argument
 * `valloc(0x20000)` is the real call (m2c over-count rule).
 */
typedef struct
{
    s16 x;                        /* 0x0 */
    s16 y;                        /* 0x2 */
    s16 w;                        /* 0x4 */
    s16 h;                        /* 0x6 */
} RECT;                           /* 0x8 (PSYQ libgpu.h) */

extern RECT _gp[];

extern void *valloc(u32 size);
extern void vfree(void *p);
extern int StoreImage2(RECT *rect, u_long *p);
extern int LoadImage2(RECT *rect, u_long *p);
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
