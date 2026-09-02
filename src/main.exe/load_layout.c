#include "common.h"
#include "main.exe.h"
#include "layout_save.h"
#include "memcard.h"

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
    layout = LoadSI(SAVE_STORAGE_DISK, names[index]);
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
    leLayoutEnemy(ENEMY_LAYOUT_GAMEPLAY);
}
