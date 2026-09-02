#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

extern Humanoid *Me_MOTION_C;

void bow_shoot_logic(s16 kind, VECTOR *start)
{
    PARAM_ITEM_LAUNCH p;
    SVECTOR move;
    s16 dist;
    s32 rot;
    s16 speed;

    p.type = kind;
    p.user = Me_MOTION_C;
    p.start.vx = start->vx;
    p.start.vy = start->vy;
    p.start.vz = start->vz;
    dist = GetTargetDistance(Me_MOTION_C, 0);
    move.pad = dist;
    rot = dtR->vy;
    speed = dist;
    if (dist < 1000)
    {
        speed = 1000;
    }
    GetMoveSpeed(&move, rot, speed, 0);
    p.end.vx = p.start.vx + move.vx;
    p.end.vy = Me_MOTION_C->target->coord.t[1];
    p.end.vz = p.start.vz + move.vz;
    if (kind == ITEM_ARROW)
    {
        if (rand() % (EngageLevel + 1) != 0)
        {
            p.end.vy -= 1000;
        }
    }
    ReqItemUse(&p);
}
