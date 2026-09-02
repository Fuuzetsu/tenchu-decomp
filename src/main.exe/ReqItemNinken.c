#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemNinken(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2461, 21 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_ninken * param
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
 *     reg   $a0       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

extern void ProcItemNinken(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemNinken(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_ninken *param;
    s32 x;
    s32 z;
    s32 i;

    TAKE_ITEM_SLOT();
    param = &item->param.ninken;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemNinken);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    {
        param_korogari *param; /* Shadows the outer launch parameter in retail. */

        param = &item->param.ninken.koro;
        x = p->end.vx;
        z = p->end.vz;
        setVector(param, x, -250, z);
        item->param.ninken.koro.hint = 0;
        param->status = KORO_NORMAL;
    }
    param->slave = 0;
    param->count = 15;
    SetNowMotion(item->owner, MOT_ITEM_THROW, MOTION_MOVE_APPLY);
    return 1;
}
