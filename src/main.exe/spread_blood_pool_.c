#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "tmdfast.h"
#include "effect.h"

extern ModelType *BLOOD_POOL_MODEL_;

extern void DrawTMD(GsDOBJ2 *object, GsOT *ot, s32 mode);

void spread_blood_pool_(Humanoid *human)
{
    VECTOR scale;
    MATRIX matrix;
    SVECTOR screen;
    s32 p;
    s32 flag;
    VECTOR *position;
    s32 *chase;
    s32 timer;
    s32 height;
    s32 scaled;
    s32 depth;

    chase = human->chase;
    if (human->motion->loop >= 0 || (human->map.attrib & (MAP_WATER | MAP_WOOD)) != 0 ||
        human->chase[0] < 0)
    {
        chase[0] = 0;
        return;
    }

    if ((human->map.attrib & MAP_DAMAGE) != 0)
    {
        if ((GameClock & 0xf) == 0)
        {
            spawn_damage_effect_(human, DAMAGE_EFFECT_ATTACHED_FLASH);
        }
        return;
    }

    timer = human->chase[0] + 0x88;
    human->chase[0] = timer;
    if (timer > FIXED_ONE)
    {
        human->chase[0] = FIXED_ONE;
    }

    position = GetAbsolutePosition(human->model->object[MODEL_PART_WAIST], 0, 0, 0);
    height = human->model->rotate.pad;
    position->vy = human->model->locate.coord.t[1];
    BLOOD_POOL_MODEL_->locate.coord.t[0] = position->vx;
    BLOOD_POOL_MODEL_->locate.coord.t[1] = position->vy;
    BLOOD_POOL_MODEL_->locate.coord.t[2] = position->vz;

    scaled = human->chase[0] * -height / 1024;
    scale.vx = scale.vy = scale.vz =
        scaled - (human->map.height >> 1);

    if (human->map.angleH != 0)
    {
        BLOOD_POOL_MODEL_->rotate.vx = 0x100;
        BLOOD_POOL_MODEL_->rotate.vy = RefrectVector[human->map.angleH];
        BLOOD_POOL_MODEL_->rotate.vz = 0;
    }
    else
    {
        BLOOD_POOL_MODEL_->rotate.vx = 0;
        BLOOD_POOL_MODEL_->rotate.vy = 0;
        BLOOD_POOL_MODEL_->rotate.vz = 0;
    }

    RotMatrixYXZ(&BLOOD_POOL_MODEL_->rotate,
                 &BLOOD_POOL_MODEL_->locate.coord);
    ScaleMatrix(&BLOOD_POOL_MODEL_->locate.coord, &scale);
    BLOOD_POOL_MODEL_->locate.flg = 0;
    GsGetLs(&BLOOD_POOL_MODEL_->locate, &matrix);
    GsSetLsMatrix(&matrix);
    depth = RotTransPers(&UnitVector, (s32 *)&screen, &p, &flag);
    screen.vz = depth;
    if ((s16)depth >> 2 < DEPTH_LIMIT)
    {
        DrawTMDmode = TMD_BANK_FOG;
        DrawTMD(&BLOOD_POOL_MODEL_->object, OTablePt, 0);
    }
}
