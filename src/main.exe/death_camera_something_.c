#include "common.h"
#include "main.exe.h"
#include "item.h"

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

extern ModelType *LOCAL_COORDINATES_;

extern void spawn_damage_effect_(Humanoid *human, s32 mode);
extern void DrawTMD(GsDOBJ2 *object, GsOT *ot, s32 mode);

void death_camera_something_(Humanoid *human)
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
    if (human->motion->loop >= 0 || (human->map.attrib & 0xc) != 0 ||
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
    LOCAL_COORDINATES_->locate.coord.t[0] = position->vx;
    LOCAL_COORDINATES_->locate.coord.t[1] = position->vy;
    LOCAL_COORDINATES_->locate.coord.t[2] = position->vz;

    scaled = human->chase[0] * -height;
    if (scaled < 0)
    {
        scaled += 0x3ff;
    }
    scale.vx = scale.vy = scale.vz =
        (scaled >> 10) - (human->map.height >> 1);

    if (human->map.angleH != 0)
    {
        LOCAL_COORDINATES_->rotate.vx = 0x100;
        LOCAL_COORDINATES_->rotate.vy = RefrectVector[human->map.angleH];
        LOCAL_COORDINATES_->rotate.vz = 0;
    }
    else
    {
        LOCAL_COORDINATES_->rotate.vx = 0;
        LOCAL_COORDINATES_->rotate.vy = 0;
        LOCAL_COORDINATES_->rotate.vz = 0;
    }

    RotMatrixYXZ(&LOCAL_COORDINATES_->rotate,
                 &LOCAL_COORDINATES_->locate.coord);
    ScaleMatrix(&LOCAL_COORDINATES_->locate.coord, &scale);
    LOCAL_COORDINATES_->locate.flg = 0;
    GsGetLs(&LOCAL_COORDINATES_->locate, &matrix);
    GsSetLsMatrix(&matrix);
    depth = RotTransPers(&UnitVector, (s32 *)&screen, &p, &flag);
    screen.vz = depth;
    if ((depth << 16) >> 18 < 0x4e2)
    {
        DrawTMDmode = 0x20;
        DrawTMD(&LOCAL_COORDINATES_->object, OTablePt, 0);
    }
}
