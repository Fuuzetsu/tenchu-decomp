#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>
#include "effect.h"

extern void SetPolyXF4(POLY_XF4 *ply, short attrib);
extern void AddXF4(void *ot, POLY_XF4 *ply);

void draw_fade_(TEffectSlot *ef)
{
    FadeType *fade;
    POLY_XF4 local;
    POLY_XF4 *ply;
    long elapsed;
    u32 duration;
    u8 r;
    u8 g;
    u8 b;
    u8 mode;

    fade = &ef->param.fade;
    SetPolyXF4(&local, GPU_BLEND_ADD);
    local.ply.x0 = -SCREEN_W / 2;
    local.ply.y0 = -SCREEN_H / 2;
    local.ply.x1 = SCREEN_W / 2;
    local.ply.y1 = -SCREEN_H / 2;
    local.ply.x2 = -SCREEN_W / 2;
    local.ply.y2 = SCREEN_H / 2;
    local.ply.x3 = SCREEN_W / 2;
    local.ply.y3 = SCREEN_H / 2;

    mode = fade->mode;
    elapsed = GameClock - fade->start_time;
    duration = fade->end_time - fade->start_time;
    switch (mode)
    {
    case FADE_MODE_IN:
        r = (elapsed * fade->r) / duration;
        g = (elapsed * fade->g) / duration;
        b = (elapsed * fade->b) / duration;
        local.ply.r0 = r;
        local.ply.g0 = g;
        local.ply.b0 = b;
        if ((u32)GameClock >= (u32)fade->end_time)
        {
            fade->mode++;
            fade->end_time += 3;
        }
        break;
    case FADE_MODE_HOLD:
        local.ply.r0 = fade->r;
        local.ply.g0 = fade->g;
        local.ply.b0 = fade->b;
        if ((u32)GameClock >= (u32)fade->end_time)
        {
            fade->mode++;
            fade->end_time += 0x28;
        }
        break;
    case FADE_MODE_OUT:
        if ((u32)GameClock >= (u32)fade->end_time)
        {
            ef->proc = 0;
            return;
        }
        r = ((duration - elapsed) * fade->r) / duration;
        g = ((duration - elapsed) * fade->g) / duration;
        b = ((duration - elapsed) * fade->b) / duration;
        local.ply.r0 = r;
        local.ply.g0 = g;
        local.ply.b0 = b;
        break;
    }

    ply = (POLY_XF4 *)GsGetWorkBase();
    GsSetWorkBase(ply + 1);
    *ply = local;
    AddXF4(OTablePt->org + fade->priority, ply);
}
