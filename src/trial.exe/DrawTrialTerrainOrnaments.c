#include "common.h"
#include "mission_editor.h"
#include "../main.exe/tmdfast.h"
#include "../main.exe/item.h"
#include "../main.exe/model.h"

extern s32 SlightPoint;

/* Ornaments share the theme's model instances. Move each instance into
 * place, reject distant/offscreen origins, and preserve packet headroom. */
void DrawTrialTerrainOrnaments(void)
{
    MATRIX matrix;
    GsOT ot = *OTablePt;
    SVECTOR screen;
    short i, model_rotation;
    int camera_x, camera_z, depth;
    OrnamentType *ornament;

    if (SkipFrame == 1)
        return;
    ot.org += 55;
    camera_x = CamState.Owner->locate->vx;
    camera_z = CamState.Owner->locate->vz;
    for (i = 0; i < TrialTerrainOrnamentCount; i++)
    {
        model_rotation = TrialTerrainOrnaments[i].model_rotation;
        ornament = TrialThemeOrnaments[model_rotation & 0xff];
        ornament->locate.super = &World.locate;
        if (__builtin_abs(TrialTerrainOrnaments[i].x - camera_x) > 17000 ||
            __builtin_abs(TrialTerrainOrnaments[i].z - camera_z) > 17000)
            continue;
        ornament->locate.coord.t[0] = TrialTerrainOrnaments[i].x;
        ornament->locate.coord.t[1] = TrialTerrainOrnaments[i].y;
        ornament->locate.coord.t[2] = TrialTerrainOrnaments[i].z;
        UpdateOrnament(ornament, (model_rotation >> 8) * 1024);
        GsGetLs(&ornament->locate, &matrix);
        GsSetLsMatrix(&matrix);
        depth = RotTransPers(&UnitVector, (long *)&screen, NULL, NULL) >> 2;
        if ((u32)(GsGetWorkBase() - Packet[DrawingPage]) > TRIAL_PACKET_PAGE_SIZE - 4000)
            return;
        if (depth != 0 && depth <= 1250 &&
            __builtin_abs(screen.vx) <= 320 && __builtin_abs(screen.vy) <= 240)
        {
            DrawTMDmode = (depth <= SlightPoint) << 5;
            DrawTMD(&ornament->object, &ot, 0);
        }
    }
}
