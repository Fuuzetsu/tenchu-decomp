#include "common.h"
#include "main.exe.h"
#include "graphics.h"
#include "item.h"
#include <psxsdk/libgpu.h>

extern char str_player_jump[];
extern char fmt_jump_x[];
extern char fmt_jump_y[];
extern char fmt_jump_z[];

void debug_menu_player_jump(void)
{
    VECTOR pos;
    u32 pad;
    u32 exit_pad;
    Humanoid *player;

    pos.vx = StagePlayer->locate->vx / 1000;
    pos.vy = StagePlayer->locate->vy / 1000;
    pos.vz = StagePlayer->locate->vz / 1000;
    EndDrawing(-2);

    while (1)
    {
        StartDrawing();
        FntPrint(str_player_jump);
        FntPrint(fmt_jump_x, pos.vx);
        FntPrint(fmt_jump_y, pos.vy);
        FntPrint(fmt_jump_z, pos.vz);
        FntFlush(-1);
        EndDrawing(-2);
        pad = GetRealPad(PAD_PORT_1);
        exit_pad = pad;
        if (pad & PADselect)
        {
            break;
        }
        if (exit_pad & PADstart)
        {
            break;
        }
        if (pad & PADLup)
            pos.vx--;
        if (pad & PADLdown)
            pos.vx++;
        if (pad & PADLright)
            pos.vz--;
        if (pad & PADLleft)
            pos.vz++;
        if (pad & PADL1)
            pos.vy--;
        if (pad & PADL2)
            pos.vy++;
    }

    if (exit_pad & PADstart)
    {
        pos.vy = GetAreaMapLevel(GlobalAreaMap,
                                 pos.vx *= 1000,
                                 pos.vy *= 1000,
                                 pos.vz *= 1000,
                                 AREA_LEVEL_STEP_DOWN);
        if (pos.vy != LEVEL_NONE)
        {
            player = StagePlayer;
            ViewInfo.vpx = ViewInfo.vrx = player->locate->vx = pos.vx;
            ViewInfo.vpy = ViewInfo.vry = player->locate->vy = pos.vy;
            ViewInfo.vpz = ViewInfo.vrz = player->locate->vz = pos.vz;
            GsSetRefView2(&ViewInfo);
        }
    }

    do
    {
        pad = GetRealPad(PAD_PORT_1);
    } while (pad != 0);
    StartDrawing();
}
