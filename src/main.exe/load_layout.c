#include "common.h"
#include "main.exe.h"

/*
 * load_layout (0x8003cc78, 0x8c bytes) - sibling of FUN_8003cd04 (the very
 * next function in this TU): loads a built-in enemy/item layout blob via
 * LoadSI (always target 0) using one of three known filenames,
 * copied from the 3-entry table `D_80012168` into a local array first (the
 * asm loads all three words up front and stores them to the stack BEFORE
 * indexing with the variable `index` - a real local-array copy, not a
 * direct `D_80012168[index]` index, which would materialize the address
 * differently), then restores the enemy layout and item layout out of it
 * and frees the buffer; reports an error via AdtMessageBox on a NULL
 * buffer (same shape as FUN_8003cd04). Unlike FUN_8003cd04, this always
 * re-lays-out the enemy table via leLayoutEnemy(1) at the end, even on a
 * failed load.
 */
extern void *LoadSI(int target, u8 *name);
extern void leRestoreEnemyLayout(void *buf);
extern void RestoreItemLayout(void *buf);
extern void vfree(void *buf);
extern void AdtMessageBox(char *fmt, ...);
extern char D_80012154[]; /* "load layout error" */
extern u8 *D_80012168[3];

void load_layout(s32 index)
{
    void *buf;
    u8 *names[3];

    __builtin_memcpy(names, D_80012168, sizeof(names));
    buf = LoadSI(0, names[index]);
    if (buf == 0) {
        AdtMessageBox(D_80012154);
    } else {
        leRestoreEnemyLayout(buf);
        RestoreItemLayout((u8 *)buf + 0x1388);
        vfree(buf);
    }
    leLayoutEnemy(1);
}
