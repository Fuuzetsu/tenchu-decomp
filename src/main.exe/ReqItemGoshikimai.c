#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemGoshikimai(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2262, 21 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s2       struct param_goshikimai * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

extern void ProcItemGoshikimai(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemGoshikimai(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_goshikimai *param;
    s32 i;

    TAKE_ITEM_SLOT();
    param = &item->param.goshikimai;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemGoshikimai);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    item->param.goshikimai.vec.vx = p->end.vx;
    param->vec.vy = p->end.vy;
    param->vec.vz = p->end.vz;
    return 1;
}
