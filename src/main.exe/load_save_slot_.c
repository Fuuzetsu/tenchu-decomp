#include "common.h"
#include "main.exe.h"

/*
 * load_save_slot_ (0x8003cd04, 0x58 bytes) — file-menu load handler: loads a
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
extern char msg_load_layout_error[]; /* load layout error */

void load_save_slot_(int target, u8 *name)
{
    void *buf;

    buf = LoadSI(target & 0xFF, name);
    if (buf == 0)
    {
        AdtMessageBox(msg_load_layout_error);
    }
    else
    {
        leRestoreEnemyLayout(buf);
        RestoreItemLayout((u8 *)buf + 0x1388);
        vfree(buf);
    }
}
