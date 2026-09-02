#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackFire(short sfrm, short efrm);
 *     MOTION.C:876, 16 src lines, frame 72 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short sfrm
 *     param $a1       short efrm
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 *     stack sp+56     struct SVECTOR vect
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;
extern int ReqItemUse(PARAM_ITEM_LAUNCH *p);

void AttackFire(s16 sfrm, s16 efrm)
{
    VECTOR *start_pos;
    PARAM_ITEM_LAUNCH item;
    SVECTOR vect;
    s16 count;

    count = dtM->count;
    if (sfrm <= count && count <= efrm)
    {
        if (count == sfrm)
        {
            Sound(Me_MOTION_C, SE_FIRE);
        }
        item.type = ITEM_NAPALM;
        item.user = Me_MOTION_C;
        start_pos = GetAbsolutePosition(
            Me_MOTION_C->model->object[MODEL_PART_HEAD], 0, -100, -300);
        item.start.vx = start_pos->vx;
        item.start.vy = start_pos->vy;
        item.start.vz = start_pos->vz;
        GetMoveSpeed(&vect, dtR->vy, 100, 0);
        item.end.vx = item.start.vx + vect.vx;
        item.end.vy = item.start.vy;
        item.end.vz = item.start.vz + vect.vz;
        ReqItemUse(&item);
    }
}
