#include "common.h"
#include "main.exe.h"
#include "layout_save.h"

/*
 * load_layout (0x8003cc78, 0x8c bytes) - sibling of load_save_slot_ (the very
 * next function in this TU): loads a built-in enemy/item layout blob via
 * LoadSI (always target 0) using one of three known filenames,
 * copied from the N_STAGE_LAYOUTS-entry table `LayoutNames` into a local
 * array first (the asm loads all three words up front and stores them to the
 * stack BEFORE
 * indexing with the variable `index` - a real local-array copy, not a
 * direct `LayoutNames[index]` index, which would materialize the address
 * differently), then restores the enemy layout and item layout out of it
 * and frees the buffer; reports an error via AdtMessageBox on a NULL
 * buffer (same shape as load_save_slot_). Unlike load_save_slot_, this always
 * re-lays-out the enemy table via leLayoutEnemy(1) at the end, even on a
 * failed load.
 */
extern void *LoadSI(int target, u8 *name);
extern void leRestoreEnemyLayout(void *buf);
extern void RestoreItemLayout(void *buf);
extern void vfree(void *buf);
extern void AdtMessageBox(char *fmt, ...);
extern char msg_load_layout_error[]; /* load layout error */
extern u8 *LayoutNames[N_STAGE_LAYOUTS];

void load_layout(s32 index)
{
    LayoutSaveData *layout;
    u8 *names[N_STAGE_LAYOUTS];

    __builtin_memcpy(names, LayoutNames, sizeof(names));
    layout = LoadSI(0, names[index]);
    if (layout == 0)
    {
        AdtMessageBox(msg_load_layout_error);
    }
    else
    {
        leRestoreEnemyLayout(layout->enemies);
        RestoreItemLayout(layout->items);
        vfree(layout);
    }
    leLayoutEnemy(1);
}
