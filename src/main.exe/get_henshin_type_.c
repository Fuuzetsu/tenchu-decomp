#include "common.h"
#include "main.exe.h"
#include "item.h"

u8 get_henshin_type_(short chr, short idx)
{
    int flag;

    flag = (chr == AYAME_0);
    return HensinT[idx][flag];
}
