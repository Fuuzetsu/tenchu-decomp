#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * ReqItemStay (0x8004a2ec) — spawn a stationary/placed item (e.g. from
 * DebugMenuItemSet/LoadConstruction/RestoreItemLayout: level-authored item
 * placements, not a runtime drop). Builds a PARAM_ITEM_USE on the stack from
 * the PARAM_ITEM_STAY input (type + locate only; no end/toss vector) and
 * forwards to ReqItemDrop, which does the actual pool allocation. Confirms
 * item.h's existing `PARAM_ITEM_STAY *p` prototype (Ghidra's seed builds
 * exactly this local + forwarding call).
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - Ghidra's call `GetAreaMapLevel(GlobalAreaMap, x, y)` only shows 2 of the
 *    5 args (the "m2c/Ghidra disagree on ARG COUNT" rule) — the raw .s loads
 *    a1/a2 from the stack AND passes u.start.vz through $a3 (still live from
 *    its own load) and a literal 0 stack arg at sp+0x10 (the 5th arg's stack
 *    slot, right after the 4-word $a0-$a3 shadow area) — same 5-arg
 *    prototype as ReqItemDrop's own call.
 *  - `type = p->type;` MUST be the first statement (matching Ghidra's order),
 *    not `user = (Humanoid *)1;` even though the latter has no dependency and
 *    looks like a more natural "trivial constant first" ordering: with the
 *    constant assignment first, its dependency-free `li v0,1` floats all the
 *    way above the prologue's `sw ra` (confirmed by compiling it that way —
 *    the ra-save no longer stays instruction 2). Putting the `p->type` load
 *    first anchors the schedule so `sw ra` stays put; the constant's `li`
 *    then fills the load's delay slot and its store still lands before the
 *    load's store (scheduler reorders the two independent stores), matching
 *    the target's `sw v0,0x1c` / `sw v1,0x18` order for free.
 */
extern long GetAreaMapLevel(void *map, long x, long y, long z, long e);
extern void *GlobalAreaMap;

int ReqItemStay(PARAM_ITEM_STAY *p)
{
    PARAM_ITEM_USE u;

    u.type = p->type;
    u.user = (Humanoid *)1;
    u.start.vx = p->locate.vx;
    u.start.vy = p->locate.vy;
    u.start.vz = p->locate.vz;
    u.end.vx = 0;
    u.end.vy = 0;
    u.end.vz = 0;
    u.start.vy = GetAreaMapLevel(GlobalAreaMap, u.start.vx, u.start.vy, u.start.vz, 0);
    ReqItemDrop(&u);
    return 1;
}
