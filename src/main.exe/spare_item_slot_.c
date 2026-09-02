#include "common.h"
#include "main.exe.h"
#include "item.h"

extern void AdtMessageBox(char *fmt, ...);
extern char fmt_not_support_yet[]; /* not support yet %d */

s32 spare_item_slot_(enum spare_item_slot_operation operation, Humanoid *human)
{
    switch (operation)
    {
    case SPARE_ITEM_SLOT_CLEAR:
    {
        Humanoid *p = human;
        if (p == 0)
            p = CamState.Owner;
        p->item[ITEM_N] = 0;
        break;
    }
    case SPARE_ITEM_SLOT_QUERY:
        if (human == 0)
            human = CamState.Owner;
        return human->item[ITEM_N] == 1;
    default:
        AdtMessageBox(fmt_not_support_yet, operation);
        break;
    }
    return 0;
}
