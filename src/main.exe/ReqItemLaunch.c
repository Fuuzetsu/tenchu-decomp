#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"
#include "afterimage.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqItemLaunch(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:3289, 25 src lines, frame 48 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s1       struct tag_TItem * item
 *     reg   $s2       struct param_launch * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $s0       struct VECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct ModelType *SyurikenModel;
 * END PSX.SYM */

extern void ProcItemLaunch(TItem *item);
extern void SetupFly(param_fly *param, VECTOR *start, VECTOR *end, s32 a4, s32 a5, s32 a6);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemLaunch(PARAM_ITEM_LAUNCH *p)
{
    TItem *item;
    TItem *ret;
    param_launch *param;
    VECTOR *pos;
    AfterimageType *ai;
    s32 i;

    TAKE_ITEM_SLOT_VIA_CURSOR(found);
found:
    param = &item->param.launch;
    if (item == 0)
        return 0;
    {
        Humanoid *aowner;
        s32 atype;

        INITIALIZE_ITEM_FROM_REQUEST(ProcItemLaunch);
        item->collision.size = 0;
        item->model = SyurikenModel;
    }
    SetupFly(&param->fly, pos, &p->end, FIXED_QUARTER, FIXED_QUARTER, 300);
    item->param.launch.fly.mode = FLY_MODE_ARC;
    ai = SetupAfterimage(item->model, 10);
    param->effect = ai;
    ai->vector1.vx = 0x14;
    ai->vector1.vy = 0;
    ai->vector1.vz = 0;
    ai->vector2.vx = -0x14;
    ai->vector2.vy = 0;
    ai->vector2.vz = 0;
    param->count = 5;
    return 1;
}
