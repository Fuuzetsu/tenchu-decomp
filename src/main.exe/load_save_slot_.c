#include "common.h"
#include "main.exe.h"
#include "layout_save.h"
#include "memcard.h"

extern void leRestoreEnemyLayout(void *buf);
extern void RestoreItemLayout(void *buf);
extern void vfree(void *buf);
extern void AdtMessageBox(char *fmt, ...);
extern char msg_load_layout_error[]; /* load layout error */

void load_save_slot_(enum save_storage storage, u8 *name)
{
    LayoutSaveData *layout;

    layout = LoadSI(storage & 0xFF, name);
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
}
