#include "common.h"
#include "main.exe.h"

void clamp_shop_stock_(TLinkInfo *ps)
{
    int i;

    for (i = 0; i < N_SHOP_ITEMS; i++)
    {
        int mx = SHOP_ITEM_DEFAULTS[i].maxStock;

        if (ps->gItem[ps->CharType][SHOP_ITEM_DEFAULTS[i].itemIndex] != ITEM_LOCKED && mx < ps->gItem[ps->CharType][SHOP_ITEM_DEFAULTS[i].itemIndex])
        {
            ps->gItem[ps->CharType][SHOP_ITEM_DEFAULTS[i].itemIndex] = mx;
        }
    }
}
