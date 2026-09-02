#include "common.h"
#include "main.exe.h"

void ResetInventory(void)
{
    s16 i;

    ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->selItem[ITEM_KAGINAWA] = ITEM_INFINITE;
    i = ITEM_SHURIKEN;
    do
    {
        ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->selItem[i] = 0;
        i++;
    } while (i < ITEM_NEMURI);
    while (i < N_LOADOUT_ITEMS)
    {
        ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->selItem[i] = ITEM_LOCKED;
        i++;
    }
}
