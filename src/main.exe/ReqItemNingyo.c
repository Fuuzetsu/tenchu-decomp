#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemNingyo(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2018, 24 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct param_ningyo * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $a0       struct VECTOR * pos
 *     reg   $s1       struct param_korogari * param
 *     reg   $v0       int x
 *     reg   $v1       int y
 *     reg   $a0       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

extern void ProcItemNingyo(TItem *item);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemNingyo(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    param_ningyo *param;
    s32 x;
    s32 y;
    s32 z;
    s32 i;

    TAKE_ITEM_SLOT();
    param = &item->param.ningyo;
    if (item == 0)
        return 0;
    {
        VECTOR *pos;
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemNingyo);
        item->collision.size = 0;
        item->model = (ModelType *)ItemImage[item->type];
    }
    {
        param_korogari *param; /* Shadows the outer launch parameter in retail. */

        param = &item->param.ningyo.koro;
        x = p->end.vx;
        y = p->end.vy;
        z = p->end.vz;
        setVector(param, x, y, z);
        item->param.ningyo.koro.hint = 0;
        param->status = KORO_NORMAL;
    }
    param->count = NINGYO_DURATION;
    item->locate->rotate.vx = 0;
    item->locate->rotate.vy = rand() % ANGLE_FULL;
    item->locate->rotate.vz = rand() % 68;
    param->hp = NINGYO_HP;
    SetNowMotion(item->owner, MOT_ITEM_THROW, MOTION_MOVE_APPLY);
    return 1;
}
