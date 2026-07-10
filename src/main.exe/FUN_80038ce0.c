#include "common.h"
#include "main.exe.h"

/*
 * FUN_80038ce0 (0x80038ce0) — screen-clear helper: calls the PSYQ libgpu
 * ClearImage BIOS-linked primitive over a fixed static RECT (D_80097A60,
 * referenced nowhere else — a private display-area scratch value for this
 * call only), clear color (0,0,0). Callers use it as a plain void-void
 * screen-clear (see BriefingAndInventorySelectionScreen's
 * `extern void FUN_80038ce0(void);`). No libgpu.h is vendored in this repo,
 * so RECT/ClearImage are declared locally per the project's per-TU extern
 * convention (see AdtSelect.c's own `extern void DrawPrim(u8 *prim);`).
 */
typedef struct
{
    s16 x;                        /* 0x0 */
    s16 y;                        /* 0x2 */
    s16 w;                        /* 0x4 */
    s16 h;                        /* 0x6 */
} RECT;                           /* 0x8 (PSYQ libgpu.h) */

extern int ClearImage(RECT *rect, u_char r, u_char g, u_char b);
extern RECT D_80097A60;

void FUN_80038ce0(void)
{
    ClearImage(&D_80097A60, 0, 0, 0);
}
