#include "common.h"
#include "main.exe.h"

/*
 * FUN_8003cd04 (0x8003cd04, 0x58 bytes) — file-menu load handler: loads a
 * save-slot buffer via LoadSI, then restores the enemy layout and item
 * layout out of it and frees the buffer; reports an error via
 * AdtMessageBox on a NULL buffer.
 *
 * param_2 is never touched in this function — it's a passthrough second
 * arg forwarded straight to LoadSI (the leading-live-in-register the
 * cookbook warns m2c can't see: m2c's reference below shows LoadSI with
 * only ONE argument since $a1 is never locally defined). FileOption.c
 * already declares `extern void FUN_8003cd04(s32 sel, s32 no);` and calls
 * `FUN_8003cd04(sel & 0xFF, no)` — two arguments, confirming param_2 is
 * real (per-TU prototypes may disagree in spelling; not "fixed" here).
 */
extern void *LoadSI(s32 slot, void *buf);
extern void leRestoreEnemyLayout(void *buf);
extern void RestoreItemLayout(void *buf);
extern void vfree(void *buf);
extern void AdtMessageBox(char *fmt, ...);
extern char D_80012154[]; /* "load layout error" */

void FUN_8003cd04(s32 param_1, void *param_2)
{
    void *buf;

    buf = LoadSI(param_1 & 0xFF, param_2);
    if (buf == 0) {
        AdtMessageBox(D_80012154);
    } else {
        leRestoreEnemyLayout(buf);
        RestoreItemLayout((u8 *)buf + 0x1388);
        vfree(buf);
    }
}
