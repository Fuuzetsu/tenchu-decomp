#include "common.h"
#include "main.exe.h"

/*
 * load_balma_area_map_ (0x8001ab64) — load a new area-map (`adr`) into the
 * SAVED cursor slot (BalmaAreaMap) without disturbing the currently-active one
 * (GlobalAreaMap/FieldIndex/FieldArea are all restored to their pre-call
 * values afterward). Same original TU as GetAreaMapLevel.c/LoadAreaMap.c/
 * swap_balma_area_map_.c (shares NodeIndexType and all four
 * globals, all %gp_rel here too) — swap_balma_area_map_ is
 * the counterpart that later swaps BalmaAreaMap back into the live slot.
 * `adr` is never reassigned, so it's simply left in $a0 across the call (no
 * explicit move) — LoadAreaMap(adr)'s result is kept live in $v0 until the
 * third store (BalmaAreaMap), after GlobalAreaMap/FieldIndex and before
 * FieldArea's pre-loaded cur->index, matching the
 * actual store order; only FieldArea's read of `cur->index` schedules early
 * (independent load hoisting past the intervening stores, same as
 * ReqItemKusuri's it->locate cookbook rule).
 */

extern AreaMapType *LoadAreaMap(AreaMapType *adr);

AreaMapType *load_balma_area_map_(AreaMapType *adr)
{
    NodeIndexType *cur;
    AreaMapType *newmap;

    cur = (NodeIndexType *)GlobalAreaMap;
    newmap = LoadAreaMap(adr);
    GlobalAreaMap = (AreaMapType *)cur;
    FieldIndex = cur;
    BalmaAreaMap = newmap;
    FieldArea = (AreaNodeType *)cur->index;
    return newmap;
}
