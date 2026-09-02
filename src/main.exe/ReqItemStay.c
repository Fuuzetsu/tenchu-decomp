#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemStay(struct PARAM_ITEM_STAY *p);
 *     ITEM.C:1140, 27 src lines, frame 64 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct PARAM_ITEM_STAY * p
 *     stack sp+16     struct PARAM_ITEM_LAUNCH param
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

int ReqItemStay(PARAM_ITEM_STAY *p)
{
    PARAM_ITEM_LAUNCH param;

    param.type = p->type;
    param.user = (Humanoid *)CONFLICT_OWNER_ITEM;
    param.start.vx = p->locate.vx;
    param.start.vy = p->locate.vy;
    param.start.vz = p->locate.vz;
    param.end.vx = 0;
    param.end.vy = 0;
    param.end.vz = 0;
    param.start.vy = GetAreaMapLevel(GlobalAreaMap, param.start.vx,
                                     param.start.vy, param.start.vz,
                                     AREA_LEVEL_DEFAULT);
    ReqItemDrop(&param);
    return 1;
}
