#include "common.h"
#include "main.exe.h"

/*
 * load_layout (0x8003cc78, 0x8c bytes) - sibling of FUN_8003cd04 (the very
 * next function in this TU): loads a built-in enemy/item layout blob via
 * LoadSI (always slot 0) into one of three known destination pointers,
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
typedef struct
{
    void *ptr[3];
} LayoutBufTable; /* 0xC */

extern void *LoadSI(s32 slot, void *buf);
extern void leRestoreEnemyLayout(void *buf);
extern void RestoreItemLayout(void *buf);
extern void vfree(void *buf);
extern void AdtMessageBox(char *fmt, ...);
extern char D_80012154[]; /* "load layout error" */
extern LayoutBufTable D_80012168;
extern void leLayoutEnemy(s32 num);

void load_layout(s32 index)
{
    void *buf;
    LayoutBufTable local_18;

    local_18 = D_80012168;
    buf = LoadSI(0, local_18.ptr[index]);
    if (buf == 0) {
        AdtMessageBox(D_80012154);
    } else {
        leRestoreEnemyLayout(buf);
        RestoreItemLayout((u8 *)buf + 0x1388);
        vfree(buf);
    }
    leLayoutEnemy(1);
}
