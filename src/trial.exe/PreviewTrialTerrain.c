#include "common.h"
#include "mission_editor.h"
#include "../main.exe/item.h"
#include "../main.exe/humanoid.h"
#include "../main.exe/sound.h"

/* Hold L1 to look around from the selected cell. The renderer only reads
 * this temporary camera owner's position, backed by ViewInfo's eye vector. */
void PreviewTrialTerrain(short x, short z)
{
    Humanoid camera_owner;
    int turn_speed = 0;
    int vertical_offset = 0;
    int angle = TrialEditorViewAngle;
    short base_height, i, corner_angle;
    u_long buttons;
    DVECTOR *vertex;

    CamState.Owner = &camera_owner;
    camera_owner.locate = (VECTOR *)&ViewInfo.vpx;
    UpdateTrialTerrainChunks(1);
    base_height = GetTrialTerrainHeight(x, z, TRIAL_HEIGHT_GROUND);
    ViewInfo.vpx = x * 2000 + 1000;
    ViewInfo.vpy = base_height =
        __builtin_abs(base_height) != 99 ? (base_height - 2) * 1000 : -2000;
    ViewInfo.vpz = z * -2000 - 1000;
    buttons = GetRealPad(0);
    while (buttons & PADL1)
    {
        i = 0;
        StartDrawing();
        ViewInfo.vrx = ViewInfo.vpx + rsin(angle);
        ViewInfo.vry = base_height + vertical_offset;
        ViewInfo.vrz = ViewInfo.vpz + rcos(angle);
        GsSetRefView2(&ViewInfo);
        DrawTrialTerrainChunks(0);
        DrawTrialTerrainOrnaments();

        /* The quad interleaves each XY pair with four texture bytes. */
        vertex = (DVECTOR *)&TrialPreviewDirectionQuad.x0;
        for (; i < 4; i++, vertex += 2)
        {
            corner_angle = (i & 1) ? 512 : 1536;
            corner_angle = (i & 2) ? angle - corner_angle : corner_angle + angle;
            vertex->vx = (rcos(corner_angle) * 160) >> 12;
            vertex->vy = 120 - ((rsin(corner_angle) * 80) >> 12);
        }
        GsSortPoly(&TrialPreviewDirectionQuad, OTablePt, 0);
        if (GameClock % 30 == 0)
            SoundEx(NULL, SE_WARNING_BEEP);
        EndDrawing(-2);
        buttons = GetRealPad(0);
        if ((buttons & PADLleft) && turn_speed >= -49)
            turn_speed -= 5;
        else if ((buttons & PADLright) && turn_speed < 50)
            turn_speed += 5;
        else if (!(buttons & (PADLleft | PADLright)))
            turn_speed /= 2;
        if ((buttons & PADLdown) && vertical_offset >= -1999)
            vertical_offset -= 100;
        else if ((buttons & PADLup) && vertical_offset < 2000)
            vertical_offset += 100;
        angle += turn_speed;
    }
    TrialEditorViewAngle = angle;
    UpdateTrialTerrainChunks(0);
}
