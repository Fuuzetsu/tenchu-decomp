#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/*
 * FUN_80038ce0 (0x80038ce0) — screen-clear helper: calls the PSYQ libgpu
 * ClearImage BIOS-linked primitive over a fixed static RECT (D_80097A60,
 * referenced nowhere else — a private display-area scratch value for this
 * call only), clear color (0,0,0). Callers use it as a plain void-void
 * screen-clear (see BriefingAndInventorySelectionScreen's
 * `extern void FUN_80038ce0(void);`).
 */
extern RECT D_80097A60;

void FUN_80038ce0(void)
{
    ClearImage(&D_80097A60, 0, 0, 0);
}
