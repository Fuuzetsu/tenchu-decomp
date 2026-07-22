#include "common.h"
#include "main.exe.h"
#include "item.h"
#include <psxsdk/libgpu.h>

/*
 * debug_menu_player_jump (0x8005cbbc, 0x2b4 bytes) — lets the debug menu
 * move StagePlayer one map-grid unit at a time, optionally snapping the
 * requested position to the area-map floor and moving the camera target
 * with the player.
 *
 * Matching notes (all verified against the original bytes):
 *  - The four format strings need distinct fixed-address symbols. Expressing
 *    them as offsets from one base lets cc1 retain and reuse that base,
 *    whereas the target materializes each address independently.
 *  - `pos` is a stack VECTOR. Keeping each `*= 1000` in GetAreaMapLevel's
 *    argument list leaves x/y/z in the argument registers while also
 *    writing their scaled values back to the stack, exactly as the target
 *    does.
 *  - The chained player/ViewInfo assignments are intentional: their
 *    right-to-left evaluation gives the target's store order without
 *    hand-written temporaries.
 *  - `exit_pad = pad` is deliberately before the first exit test even
 *    though it is only consumed on that test's taken path. Moving the copy
 *    into the `if` leaves control flow and instruction count unchanged, but
 *    changes its allocno home and produces a two-byte register mismatch.
 *  - A bounded late permuter run found this final source with a nonzero
 *    proxy score; authoritative full-link rescoring proved it byte-exact.
 */

extern char D_800146DC[];
extern char D_800146EC[];
extern char D_800146F8[];
extern char D_80014704[];

extern s32 GetRealPad(s32 port);
extern s32 GetAreaMapLevel(u32 *area, s32 x, s32 y, s32 z, s32 mode);

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
        FntPrint(D_800146DC);
        FntPrint(D_800146EC, pos.vx);
        FntPrint(D_800146F8, pos.vy);
        FntPrint(D_80014704, pos.vz);
        FntFlush(-1);
        EndDrawing(-2);
        pad = GetRealPad(0);
        exit_pad = pad;
        if (pad & 0x100)
        {
            break;
        }
        if (pad & 0x800)
            goto move_player;
        if (pad & 0x1000)
            pos.vx--;
        if (pad & 0x4000)
            pos.vx++;
        if (pad & 0x2000)
            pos.vz--;
        if (pad & 0x8000)
            pos.vz++;
        if (pad & 4)
            pos.vy--;
        if (pad & 1)
            pos.vy++;
    }

    if (exit_pad & 0x800)
    {
move_player:
        pos.vy = GetAreaMapLevel(GlobalAreaMap,
                                 pos.vx *= 1000,
                                 pos.vy *= 1000,
                                 pos.vz *= 1000,
                                 1);
        if (pos.vy != (s32)0x80000000)
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
        pad = GetRealPad(0);
    } while (pad != 0);
    StartDrawing();
}
