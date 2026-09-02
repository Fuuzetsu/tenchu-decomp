#include "common.h"
#include "main.exe.h"
#include "sound.h"

u8 *search_id_table_(u8 *table, s32 id)
{
    while (*table != SOUND_TABLE_END)
    {
        if (id == *table)
            return table;
        table += 6;
    }
    return 0;
}
