#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "tmdfast.h"

/*
 * MATCH.
 *
 * Updates and draws the flattened model used while the humanoid is dying.
 * The timer increment belongs in the non-attribute fallthrough after the
 * returning branch: cc1 then moves it into that branch's delay slot and keeps
 * the following store/clamp sequence in the target order.  A separate timer,
 * signed scale intermediate, projection depth, and full-width model height
 * also preserve the target's register lifetimes and signed `lh` load.
 */

extern ModelType *BLOOD_POOL_MODEL_;

extern void spawn_damage_effect_(Humanoid *human, s32 mode);
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

    chase = &human->chase[0];
    if (human->motion->loop >= 0 || (human->map.attrib & (MAP_WATER | MAP_WOOD)) != 0 ||
        human->chase[0] < 0)
    {
        *chase = 0;
        return;
    }

    if ((human->map.attrib & 0x100) != 0)
    {
        if ((GameClock & 0xf) == 0)
        {
            spawn_damage_effect_(human, 0);
        }
        return;
    }

    timer = human->chase[0] + 0x88;
    human->chase[0] = timer;
    if (timer > 0x1000)
    {
        human->chase[0] = 0x1000;
    }

    position = GetAbsolutePosition(human->model->object[0], 0, 0, 0);
    height = human->model->rotate.pad;
    position->vy = human->model->locate.coord.t[1];
    BLOOD_POOL_MODEL_->locate.coord.t[0] = position->vx;
    BLOOD_POOL_MODEL_->locate.coord.t[1] = position->vy;
    BLOOD_POOL_MODEL_->locate.coord.t[2] = position->vz;

    scaled = human->chase[0] * -height;
    if (scaled < 0)
    {
        scaled += 0x3ff;
    }
    scale.vx = scale.vy = scale.vz =
        (scaled >> 10) - (human->map.height >> 1);

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
