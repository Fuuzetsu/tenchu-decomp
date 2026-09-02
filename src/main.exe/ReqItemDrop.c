#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemDrop(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:907, 15 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_drop * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *     reg   $s1       struct param_korogari * param
 *     reg   $v1       int x
 *     reg   $a0       int y
 *     reg   $a1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

extern void ProcItemDrop(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemDrop(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_drop *param;
    s32 i;

    TAKE_ITEM_SLOT();
    param = &item->param.drop;
    if (item == 0)
        return 0;
    if (GetAreaMapLevel(GlobalAreaMap, p->start.vx, p->start.vy, p->start.vz,
                        AREA_LEVEL_DEFAULT) < p->start.vy)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemDrop);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
        {
            s32 x;
            s32 y;
            s32 z;

            x = p->end.vx;
            y = p->end.vy;
            z = p->end.vz;
            param->koro.vx = x;
            param->koro.vy = y;
            param->koro.vz = z;
            item->param.drop.koro.hint = 0;
            param->koro.status = KORO_NORMAL;
        }
    }
    return 1;
}
