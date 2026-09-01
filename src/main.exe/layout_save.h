#ifndef LAYOUT_SAVE_H
#define LAYOUT_SAVE_H

#include "item.h"

/* FileOption stores the editable enemy and item tables in separate fixed-size
 * sections.  The records occupy only the beginning of each section; the
 * remaining bytes preserve the original ENESIZE/ITEMSIZE file layout. */
typedef struct LayoutSaveData
{
    TEnemyLayout enemies[MAX_ENEMIES];
    u8 enemy_reserved[ENESIZE - sizeof(TEnemyLayout) * MAX_ENEMIES];
    TItemLayout items[MAX_ITEMS];
    u8 item_reserved[ITEMSIZE - sizeof(TItemLayout) * MAX_ITEMS];
} LayoutSaveData;

#endif
