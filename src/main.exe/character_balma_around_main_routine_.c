#include "common.h"
#include "main.exe.h"

/*
 * character_balma_around_main_routine_ (0x8001aba0, 0x24 bytes) — swaps the
 * live area-map cursor (GlobalAreaMap) with a saved one (D_800976E8) and
 * refreshes FieldIndex/FieldArea from the newly-installed cursor. Called
 * twice from ControlAllHumanoid (save/restore the cached area-map state
 * around per-character processing) — the FieldIndex/FieldArea-from-cursor
 * idiom is the same one PlayerOption.c's case 0 uses.
 *
 * gp: all four globals (GlobalAreaMap, D_800976E8, FieldIndex, FieldArea)
 * are %gp_rel here, same as in GetAreaMapLevel.c — this function's original
 * TU is that same map/area-map TU. D_800976E8 is an unnamed 4-byte gp
 * global sitting between GlobalAreaMap and ConflictObjects (no config
 * symbol needed — splat auto-names it).
 */

typedef struct NodeIndexType
{
    s16 y;      /* 0x0 */
    s16 n;      /* 0x2 */
    long index; /* 0x4 — AreaNodeType*; 0 terminates */
    s16 x1;     /* 0x8 */
    s16 z1;     /* 0xA */
    s16 x2;     /* 0xC */
    s16 z2;     /* 0xE */
} NodeIndexType;

typedef struct AreaNodeType AreaNodeType; /* opaque here — only the pointer is used */

extern NodeIndexType *FieldIndex;
extern AreaNodeType *FieldArea;
extern void *GlobalAreaMap;
extern NodeIndexType *D_800976E8;

void character_balma_around_main_routine_(void)
{
    NodeIndexType *saved;
    void *cur;
    AreaNodeType *area;

    saved = D_800976E8;
    cur = GlobalAreaMap;
    area = (AreaNodeType *)saved->index;

    GlobalAreaMap = saved;
    D_800976E8 = (NodeIndexType *)cur;
    FieldIndex = saved;
    FieldArea = area;
}
