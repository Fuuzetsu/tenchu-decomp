#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/*
 * FUN_80038ce0 (0x80038ce0) — screen-clear helper: calls the PSYQ libgpu
 * ClearImage BIOS-linked primitive over a fixed static RECT (ScreenRect,
 * referenced nowhere else — a private display-area scratch value for this
 * call only), clear color (0,0,0). Callers use it as a plain void-void
 * screen-clear (see BriefingAndInventorySelectionScreen's
 * `extern void FUN_80038ce0(void);`).
 */
extern RECT ScreenRect; /* {0,0,320,480}: both pages */

void FUN_80038ce0(void)
{
    ClearImage(&ScreenRect, 0, 0, 0);
}
