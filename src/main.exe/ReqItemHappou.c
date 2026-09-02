#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"
#include "afterimage.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int ReqItemHappou(struct PARAM_ITEM_LAUNCH *p);
 *     ITEM.C:2541, 41 src lines, frame 72 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s4       struct PARAM_ITEM_LAUNCH * p
 *     reg   $s5       int i
 *     reg   $s1       struct tag_TItem * item
 *     reg   $s3       struct param_launch * param
 *     reg   $s0       struct tag_TItem * ret
 *     reg   $a1       int i
 *     reg   $a0       struct tag_TItem * item
 *     reg   $s0       struct tag_TItem * item
 *     reg   $s1       struct tag_TItem * item
 *     reg   $v1       struct Humanoid * aowner
 *     reg   $a0       int atype
 *     reg   $s2       struct VECTOR * pos
 *     stack sp+24     struct SVECTOR rot
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

extern void ProcItemHappou(TItem *item);
extern Humanoid *SearchItemTarget2(Humanoid *owner, SVECTOR *rot,
                                   VECTOR *start, VECTOR *target);
extern void SetupFly(param_fly *param, VECTOR *start, VECTOR *end, s32 a4, s32 a5, s32 a6);
extern int rand(void);
/* ITEM.C defines the counter (gp-relative): listed in Build.hs
 * maspsxGpExterns for this file, unlike ActionHalt/EmergencyNotice (absolute here). */

int ReqItemHappou(PARAM_ITEM_LAUNCH *p)
{
    enum
    {
        R = 256
    };
    TItem *item;
    TItem *ret;
    param_launch *param;
    VECTOR *pos;
    VECTOR *en;
    Humanoid *aowner;
    s32 atype;
    AfterimageType *ai;
    SVECTOR rot;
    s32 r;
    s32 i;

    SetNowMotion(p->user, MOT_ITEM_THROW, MOTION_MOVE_APPLY);
    i = 0;
    while (1)
    {
        if (i >= 8)
            break;
        {
            s32 i;

            TAKE_ITEM_SLOT_VIA_CURSOR(found);
        }

    found:
        param = &item->param.launch;
        if (item == 0)
            return 0;
        INITIALIZE_ITEM_FROM_REQUEST(ProcItemHappou);
        item->collision.size = 0;
        item->locate->rotate = p->user->model->rotate;
        rot = p->user->model->rotate;
        r = rand();
        rot.vy += (r % (R * 2) - R);
        en = &p->end;
        SearchItemTarget2(p->user, &rot, pos, en);
        SetupFly(&param->fly, pos, en, FIXED_ONE, FIXED_QUARTER, 400);
        i++;
        ai = SetupAfterimage(item->locate, 10);
        param->effect = ai;
        ai->vector1.vx = 30;
        ai->vector1.vy = 0;
        ai->vector1.vz = 0;
        ai->vector2.vx = -30;
        ai->vector2.vy = 0;
        ai->vector2.vz = 0;
        param->count = 8;
    }
    Sound(p->user, SE_ITEM_USE);
    return 1;
}
