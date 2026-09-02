#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

extern Humanoid *Me_MOTION_C;

void launch_lightning_bolt_(s16 frame)
{
    VECTOR *start_pos;
    PARAM_ITEM_LAUNCH p;
    SVECTOR move;

    if (dtM->count == frame)
    {
        Sound(Me_MOTION_C, CHAR_SE_SPECIAL);
        p.type = ITEM_LIGHTNINGBOLT;
        p.user = Me_MOTION_C;
        start_pos = GetAbsolutePosition(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0], 0, 0, -700);
        p.start.vx = start_pos->vx;
        p.start.vy = start_pos->vy;
        p.start.vz = start_pos->vz;
        GetMoveSpeed(&move, dtR->vy, ((rand() % 5) * 1000 + 4000), 0);
        p.end.vx = start_pos->vx + move.vx;
        p.end.vy = Me_MOTION_C->target->coord.t[1];
        p.end.vz = start_pos->vz + move.vz;
        ReqItemUse(&p);
    }
}
