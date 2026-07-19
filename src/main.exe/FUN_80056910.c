#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * FUN_80056910 (0x80056910, 0x12c bytes) — tiles ONE shared GsSPRITE
 * (libgs.h's proven struct, embedded at +0x68 of a larger unidentified
 * object) over a fixed on-screen rectangle. It sets orientation bits from
 * the signed low half of `dir`, writes r=g=b=abs(dir), then walks x=-160..160
 * by sp.w and y=-120..sp.h+120 by sp.h, sorting the sprite at priority 1.
 * The only caller is still-asm FUN_80055d64.
 *
 * Matching notes:
 *  - `width` and `height` must be s32. The old s16 width created a second
 *    sign-extended, call-crossing copy and forced an extra s5 save/restore
 *    (308 bytes). s32 removes that copy and recovers the target five roles:
 *    s0=sp, s1=-width, s2=width, s3=(width < -width), s4=height.
 *  - The zero-trip wrapper around the inner while is load-bearing. It raises
 *    width's loop-depth allocation score above the invariant guard
 *    (1372 vs 1200), selecting target s2/s3 instead of swapping them.
 *  - The zero-trip shade wrapper and explicit `__builtin_abs` are both
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
 *  - Keeping `dir` u16 plus the early signedDir assignment reproduces the
 *    target's prologue-time a2 copy and both signed tests. Direct g->sp
 *    attribute accesses before forming `sp`, reversed equal color stores,
 *    and the untruncated height comparisons reproduce the remaining blocks.
 */

typedef struct
{
    u8 pad0[0x68];
    GsSPRITE sp; /* +0x68 */
} SpriteGridType;

extern GsOT *OTablePt;

void FUN_80056910(SpriteGridType *g, u16 dir)
{
    GsSPRITE *sp;
    u32 flags;
    s16 signedDir;
    s32 shade;
    s32 initial_h;
    u16 h;
    s32 width;
    s32 height;

    signedDir = (s16)dir;
    width = 0xa0;
    height = 0x78;
    flags = g->sp.attribute & 0x8fffffff;
    g->sp.attribute = flags;
    g->sp.attribute = flags | (0 < signedDir ? 0x60000000 : 0x50000000);
    sp = &g->sp;
    shade = __builtin_abs((s32)signedDir);
        sp->b = (u8)shade;
        sp->g = (u8)shade;
        sp->r = (u8)shade;
    initial_h = sp->h;
    sp->y = -height;
    if (-height <= height + initial_h) {
        do {
            sp->x = -width;
            while (sp->x <= width) {
                    GsSortSprite(sp, OTablePt, 1);
                    sp->x = sp->x + sp->w;
                }
            sp->y = sp->y + sp->h;
            h = sp->h;
        } while (sp->y <= height + h);
    }
}
