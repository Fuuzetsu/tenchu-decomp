#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>
#include "effect.h"

extern void SetPolyXF4(POLY_XF4 *ply, short attrib);
extern void AddXF4(void *ot, POLY_XF4 *ply);

/*
 * STATUS: MATCH (exact).
 *
 * The old positional SetGore suggestion for this address is rejected: this
 * function renders the retail-only full-screen FadeType, not a gore particle.
 * SetGore is instead the nearby allocator at 0x80035f44, which installs
 * DrawGore as its effect callback.
 *
 * The three interpolated channels are ordinary byte-sized temporaries. Their
 * natural QImode pseudos give the target's caller-register conflicts; the old
 * mixed u32/u16/u32 draft created a false a0/a2 coloring problem.
 *
 * Case 2 writes the inverse time expression at each channel:
 * `(duration - elapsed) * color / duration`. GCC's CSE shares the repeated
 * subtraction into the target's separate v1 pseudo. Assigning the subtraction
 * back to `elapsed`, or naming a conventional `remaining` local, instead
 * ties it to a1 and leaves a 14-byte register-only residual.
 *
 * This human-scale source matches all 728 bytes without fences or donor copies.
 */

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
    SetPolyXF4(&local, 1);
    local.ply.x0 = -0xA0;
    local.ply.y0 = -120;
    local.ply.x1 = 0xA0;
    local.ply.y1 = -120;
    local.ply.x2 = -0xA0;
    local.ply.y2 = 120;
    local.ply.x3 = 0xA0;
    local.ply.y3 = 120;

    mode = fade->mode;
    elapsed = GameClock - fade->start_time;
    duration = fade->end_time - fade->start_time;
    switch (mode)
    {
    case 0:
        r = (elapsed * fade->r) / duration;
        g = (elapsed * fade->g) / duration;
        b = (elapsed * fade->b) / duration;
        local.ply.r0 = r;
        local.ply.g0 = g;
        local.ply.b0 = b;
        if ((u32)GameClock >= (u32)fade->end_time)
        {
            fade->mode++;
            fade->end_time = fade->end_time + 3;
        }
        break;
    case 1:
        local.ply.r0 = fade->r;
        local.ply.g0 = fade->g;
        local.ply.b0 = fade->b;
        if ((u32)GameClock >= (u32)fade->end_time)
        {
            fade->mode++;
            fade->end_time = fade->end_time + 0x28;
        }
        break;
    case 2:
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
