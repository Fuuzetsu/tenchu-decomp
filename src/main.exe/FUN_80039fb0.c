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
 * MATCH.
 *
 * FUN_80039fb0 (0x80039fb0, 0x198 bytes) — same "project a 3D point and
 * sort-draw a sprite there" tail as DrawSpriteXYZ/FUN_8003a148, but drawing
 * TWO sprites (`sp2`, then `sp1`) at the SAME projected point: one call to
 * GetScreenPosition computes the shared (x,y,otz), then each sprite gets its own
 * scale/rotate/position/colour and its own GsSortSprite (`sp2`'s colour is
 * `color` raw, `sp1`'s is `color / 2` — a dimmed "shadow"/second copy of the
 * same sprite, guessing from the pattern). Statement order is Ghidra's
 * literal order here (no polarity fixes needed, unlike FUN_8003a148): every
 * field write's ordering in the raw `.s` already matches Ghidra's dump
 * exactly (param_2/sp2 first, then param_1/sp1, for every field).
 *
 * Matching notes (see DrawSpriteXYZ.c for the shared idioms in full):
 *  - This TU divides by a runtime value (`size*300/otz`): needs
 *    `--expand-div` (Build.hs maspsxGpExterns' `extra` list + permute.py's
 *    MASPSX_EXTRA), same as DrawSpriteXYZ/FUN_8003a148.
 *  - The clamp expression (`(s32)((u16)out.vz << 16) >> 0x12`) is written
 *    out TWICE, once per GsSortSprite call — each occurrence independently
 *    re-reads the struct field `out.vz` (a fresh `lhu` in the target both
 *    times), rather than sharing one `t`/`pri` computation across both
 *    calls: the target's asm repeats the whole `lhu`+shift+clamp sequence
 *    verbatim for the second sort. Each needs its own `goto zero;`/`done:`
 *    labels (a plain `if/else` here is the WRONG polarity — see
 *    DrawSpriteXYZ's writeup for why the trivial `pri=0` body must be the
 *    branch TARGET, not the fallthrough).
 *  - `color / 2` for `sp1`'s r/g/b is plain signed-integer division by a
 *    CONSTANT power of two — the round-toward-zero correction
 *    (`srl v0,s2,31; addu v0,s2,v0; sra v0,v0,1`) falls straight out of
 *    `/2` on a signed value; no special-casing needed.
 *  - `out.vx`/`out.vy` need NAMED temps (`sx`/`sy`) to get ONE load reused
 *    for both sprites' `x`/`y` stores. Writing the two stores as two
 *    inline `sp2->x = out.vx; sp1->x = out.vx;` (no intervening statement
 *    at all) still produced a SECOND `lhu` in the draft — `out`'s address
 *    was taken earlier (passed to `GetScreenPosition`), and once a stack local's
 *    address escapes, cc1 stops CSEing its repeated reads across separate
 *    statements even with nothing between them. A named temp assigned once
 *    and read twice fixes it (16 bytes: 2 spurious `lhu`+`nop` pairs).
 */
extern GsOT *OTablePt;
extern void GetScreenPosition(s32 x, s32 y, s32 z, s32 *out);

void FUN_80039fb0(GsSPRITE *sp1, GsSPRITE *sp2, s32 x, s32 y, s32 z, s32 size, long rotate, s32 color)
{
    SVECTOR out;
    s32 otz;
    s16 sc;
    s16 sx;
    s16 sy;
    s32 t;
    s32 pri;

    GetScreenPosition(x, y, z, (s32 *)&out);
    otz = out.vz;
    if (otz > 0x24)
    {
        sc = (s16)((size * 300) / otz) + 1;
        sp2->scaley = sc;
        sp2->scalex = sc;
        sp1->scaley = sc;
        sp1->scalex = sc;
        sp2->rotate = rotate;
        sp1->rotate = rotate;
        sx = out.vx;
        sp2->x = sx;
        sp1->x = sx;
        sy = out.vy;
        sp2->y = sy;
        sp1->y = sy;
        sp2->r = (u8)color;
        sp2->g = (u8)color;
        sp2->b = (u8)color;
        sp1->r = (u8)(color / 2);
        sp1->g = (u8)(color / 2);
        sp1->b = (u8)(color / 2);

        t = (s32)((u16)out.vz << 16) >> 0x12;
        if (t < 0)
        {
            goto zero1;
        }
        pri = 0x4e1;
        if (t < 0x4e2)
        {
            pri = t;
        }
        goto done1;
    zero1:
        pri = 0;
    done1:
        GsSortSprite(sp2, OTablePt, (u16)pri);

        t = (s32)((u16)out.vz << 16) >> 0x12;
        if (t < 0)
        {
            goto zero2;
        }
        pri = 0x4e1;
        if (t < 0x4e2)
        {
            pri = t;
        }
        goto done2;
    zero2:
        pri = 0;
    done2:
        GsSortSprite(sp1, OTablePt, (u16)pri);
    }
}
