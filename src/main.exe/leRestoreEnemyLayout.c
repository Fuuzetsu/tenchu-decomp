#include "common.h"
#include "main.exe.h"

/*
 * leRestoreEnemyLayout (0x8003ca78, 0x2c bytes) — bulk-restore the whole
 * enemy-layout table from a caller-supplied buffer (the inverse of a save;
 * `le`=layout-enemy family, see leResetPath.c for TEnemyLayout, recovered
 * from the Ghidra type export). sizeof(enemy) == 0x1e * 0x88 == 0xFF0,
 * matching the memcpy length exactly (free at compile time).
 */

typedef struct
{
    s16 type;
    s16 ThinkType;
    s16 nPath;
    s32 x;
    s32 y;
    s32 z;
    s16 r;
    s16 pad;
    VECTOR path[7];
} TEnemyLayout; /* 0x88 */

extern TEnemyLayout enemy[0x1E];
extern void *memcpy(void *s1, void *s2, u32 n);

void leRestoreEnemyLayout(void *buf)
{
    memcpy(enemy, buf, sizeof(enemy));
}
