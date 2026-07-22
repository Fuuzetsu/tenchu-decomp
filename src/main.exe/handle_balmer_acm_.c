#include "common.h"
#include "main.exe.h"

/*
 * handle_balmer_acm_ (0x8001ab64) — load a new area-map (`adr`) into the
 * SAVED cursor slot (D_800976E8) without disturbing the currently-active one
 * (GlobalAreaMap/FieldIndex/FieldArea are all restored to their pre-call
 * values afterward). Same original TU as GetAreaMapLevel.c/LoadAreaMap.c/
 * character_balma_around_main_routine_.c (shares NodeIndexType and all four
 * globals, all %gp_rel here too) — character_balma_around_main_routine_ is
 * the counterpart that later swaps D_800976E8 back into the live slot.
 * `adr` is never reassigned, so it's simply left in $a0 across the call (no
 * explicit move) — LoadAreaMap(adr)'s result is kept live in $v0 until the
 * LAST store (D_800976E8), after GlobalAreaMap/FieldIndex, matching the
 * actual store order; only FieldArea's read of `cur->index` schedules early
 * (independent load hoisting past the intervening stores, same as
 * ReqItemKusuri's it->locate cookbook rule).
 */

extern u_long *LoadAreaMap(u_long *adr);

u_long *handle_balmer_acm_(u_long *adr)
{
    NodeIndexType *cur;
    u_long *newmap;

    cur = (NodeIndexType *)GlobalAreaMap;
    newmap = LoadAreaMap(adr);
    GlobalAreaMap = (u_long *)cur;
    FieldIndex = cur;
    D_800976E8 = newmap;
    FieldArea = (AreaNodeType *)cur->index;
    return newmap;
}
