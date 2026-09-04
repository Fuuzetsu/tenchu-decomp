#include "common.h"
#include "main.exe.h"

void ResetInventory(void)
{
    s16 i;

    ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->selItem[ITEM_KAGINAWA] = ITEM_INFINITE;
    for (i = ITEM_SHURIKEN; i < ITEM_NEMURI; i++)
    {
        ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->selItem[i] = 0;
    }
    while (i < N_LOADOUT_ITEMS)
    {
        ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->selItem[i] = ITEM_LOCKED;
        i++;
    }
}
