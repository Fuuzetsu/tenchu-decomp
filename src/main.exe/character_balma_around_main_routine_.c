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

extern NodeIndexType *FieldIndex;
extern AreaNodeType *FieldArea;
void character_balma_around_main_routine_(void)
{
    NodeIndexType *saved;
    u_long *cur;
    AreaNodeType *area;

    saved = (NodeIndexType *)D_800976E8;
    cur = GlobalAreaMap;
    area = (AreaNodeType *)saved->index;

    GlobalAreaMap = (u_long *)saved;
    D_800976E8 = cur;
    FieldIndex = saved;
    FieldArea = area;
}
