#include "common.h"
#include "main.exe.h"
#include "item.h"

extern void ProcItemGun(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

void ReqItemGun(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    VECTOR *pos;
    Humanoid *aowner;
    s32 atype;
    s32 i;

    TAKE_ITEM_SLOT();
    if (item == 0)
        return;
    INITIALIZE_ITEM_FROM_REQUEST(ProcItemGun);
    item->collision.size = 0;
    item->param.gun.vec = p->end;
}
