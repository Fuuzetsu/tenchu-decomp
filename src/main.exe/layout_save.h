#ifndef LAYOUT_SAVE_H
#define LAYOUT_SAVE_H

#include "item.h"
#include "memcard.h"

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

extern enemy_layout_index leFindEnemy(void);
extern void leLayoutEnemy(enemy_layout_mode mode);
extern void leAddPath(enemy_layout_index id, s32 x, s32 y, s32 z);
extern void leResetPath(enemy_layout_index id);
extern void leRestoreEnemyLayout(void *buf);
extern void lePackEnemyLayout(void *buf, long size);
extern int leRemoveEnemy(void);
extern enemy_layout_index leSetEnemy(s32 type, TThinkType think, s32 x,
                                     s32 y, s32 z, s16 rotation);
extern void leClearLayout(void);
extern void leResetEnemyLayout(void);
extern void load_layout(s32 index);
extern void load_save_slot_(enum save_storage storage, u8 *name);

#endif
