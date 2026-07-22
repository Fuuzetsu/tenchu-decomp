#include "common.h"
#include "main.exe.h"

/*
 * FUN_8003cd04 (0x8003cd04, 0x58 bytes) — file-menu load handler: loads a
 * save-slot buffer via LoadSI, then restores the enemy layout and item
 * layout out of it and frees the buffer; reports an error via
 * AdtMessageBox on a NULL buffer.
 *
 * `name` is never touched in this function — it's a passthrough second
 * arg forwarded straight to LoadSI (the leading-live-in-register the
 * cookbook warns m2c can't see: m2c's reference below shows LoadSI with
 * only ONE argument since $a1 is never locally defined). FileOption.c
 * passes the PSX.SYM-proven `unsigned char *fname` in that register,
 * confirming that the second argument is LoadSI's filename.
 */
extern void *LoadSI(int target, u8 *name);
extern void leRestoreEnemyLayout(void *buf);
extern void RestoreItemLayout(void *buf);
extern void vfree(void *buf);
extern void AdtMessageBox(char *fmt, ...);
extern char D_80012154[]; /* "load layout error" */

void FUN_8003cd04(int target, u8 *name)
{
    void *buf;

    buf = LoadSI(target & 0xFF, name);
    if (buf == 0) {
        AdtMessageBox(D_80012154);
    } else {
        leRestoreEnemyLayout(buf);
        RestoreItemLayout((u8 *)buf + 0x1388);
        vfree(buf);
    }
}
