#include "common.h"
#include "main.exe.h"
#include "images.h"

/*
 * tile_sprite_ (0x80056910, 0x12c bytes) — tiles a Sprite3D's embedded
 * GsSPRITE over a fixed on-screen rectangle. Its blend mode comes from
 * the signed low half of `shade`, r=g=b=abs(shade), then it walks x=-160..160
 * by sp.w and y=-120..sp.h+120 by sp.h, sorting the sprite at priority 1.
 * The only caller is game_over_screen_ (three sites).
 *
 * Matching notes:
 *  - `width` and `height` must be s32. The old s16 width created a second
 *    sign-extended, call-crossing copy and forced an extra s5 save/restore
 *    (308 bytes). s32 removes that copy and recovers the target five roles:
 *    s0=sp, s1=-width, s2=width, s3=(width < -width), s4=height.
 *  - The zero-trip wrapper around the outer y do/while is load-bearing. It raises
 *    width's loop-depth allocation score above the invariant guard
 *    (1372 vs 1200), selecting target s2/s3 instead of swapping them.
 *  - The explicit `__builtin_abs` is
 *    load-bearing. A manual sign-fix expands to a separate branch early
 *    enough for reorg to steal the preceding `sp` address calculation into
 *    its delay slot. The builtin remains one `abssi2` RTL instruction through
 *    dbr and only then emits `bgez; nop; negu`, preserving the target nop and
 *    the preceding `addiu s0,a0,104`.
 *  - The first height read needs a distinct full-width, one-use `initial_h`
 *    carrier before the y store. That lets combine emit one SI-producing lhu
 *    and sched place the independent `-height` between the load and its use.
 *    Reusing the later u16 `h` instead leaves a load-delay nop; moving that
 *    narrow carrier earlier requires an extra `andi`.
 *  - Keeping `shade` u16 plus the early signedShade assignment reproduces the
 *    target's prologue-time a2 copy and both signed tests. Direct g->sp
 *    attribute accesses before forming `sp`, reversed equal color stores,
 *    and the untruncated height comparisons reproduce the remaining blocks.
 */

void tile_sprite_(Sprite3D *g, u16 shade)
{
    GsSPRITE *sp;
    u32 flags;
    s16 signedShade;
    s32 level;
    s32 initial_h;
    u16 h;
    s32 width;
    s32 height;

    signedShade = (s16)shade;
    width = 160;
    height = 120;
    flags = g->sprite.attribute & 0x8fffffff;
    /* Dead store, but retail's own bytes (removal breaks the image): the
     * masked word is written back once plain before the blend bit lands. */
    g->sprite.attribute = flags;
    g->sprite.attribute = flags | (signedShade > 0 ? SPR_TRANS_SUB : SPR_TRANS_ADD);
    sp = &g->sprite;
    level = __builtin_abs((s32)signedShade);
    sp->b = (u8)level;
    sp->g = (u8)level;
    sp->r = (u8)level;
    initial_h = sp->h;
    sp->y = -height;
    if (-height <= height + initial_h)
    {
        do
        {
            sp->x = -width;
            while (sp->x <= width)
            {
                GsSortSprite(sp, OTablePt, 1);
                sp->x += sp->w;
            }
            sp->y += sp->h;
            h = sp->h;
        } while (sp->y <= height + h);
    }
}
