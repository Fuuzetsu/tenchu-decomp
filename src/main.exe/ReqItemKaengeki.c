#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemKaengeki(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2344, 20 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_kaengeki * param
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

extern void ProcItemKaengeki(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemKaengeki(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_kaengeki *param;
    s32 i;

    TAKE_ITEM_SLOT();
    param = &item->param.kaengeki;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemKaengeki);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    item->param.kaengeki.start.vx = p->start.vx;
    param->start.vy = p->start.vy;
    param->start.vz = p->start.vz;
    param->end.vx = p->end.vx;
    param->end.vy = p->end.vy;
    param->end.vz = p->end.vz;
    return 1;
}
