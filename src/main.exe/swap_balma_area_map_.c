#include "common.h"
#include "main.exe.h"

/*
 * swap_balma_area_map_ (0x8001aba0, 0x24 bytes) — swaps the
 * live area-map cursor (GlobalAreaMap) with a saved one (BalmaAreaMap) and
 * refreshes FieldIndex/FieldArea from the newly-installed cursor. Called
 * twice from ControlAllHumanoid (save/restore the cached area-map state
 * around per-character processing) — the FieldIndex/FieldArea-from-cursor
 * idiom is the same one PlayerOption.c's case 0 uses.
 *
 * gp: all four globals (GlobalAreaMap, BalmaAreaMap, FieldIndex, FieldArea)
 * are %gp_rel here, same as in GetAreaMapLevel.c — this function's original
 * TU is that same map/area-map TU. BalmaAreaMap is an unnamed 4-byte gp
 * global sitting between GlobalAreaMap and ConflictObjects (no config
 * symbol needed — splat auto-names it).
 */

void swap_balma_area_map_(void)
{
    NodeIndexType *saved;
    AreaMapType *cur;
    AreaNodeType *area;

    saved = (NodeIndexType *)BalmaAreaMap;
    cur = GlobalAreaMap;
    area = (AreaNodeType *)saved->index;

    GlobalAreaMap = (AreaMapType *)saved;
    BalmaAreaMap = cur;
    FieldIndex = saved;
    FieldArea = area;
}
