#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawSmoke(struct tag_EffectSlot *ef);
 *     EFFECT.C:794, 39 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s3       struct tag_EffectSlot * ef
 *     reg   $s0       struct SmokeType * smoke
 *     reg   $s1       struct Sprite3D * spr
 *     reg   $s2       unsigned char alfa
 *
 * Globals it touches, as the original declared them:
 *     extern struct Sprite3D *sprSmoke;
 * END PSX.SYM */

/*
 * MATCH.
 *
 * DrawSmoke (0x800334c4, EFFECT.C:794) — the smoke-puff effect's per-frame
 * draw: picks its sprite from `sprSmoke[param->unk22]`, and while
 * `mode==bright` (a one-shot "just spawned or freshly stepped" gate) damps
 * the velocity by 0.8x every time `vec.vy` drifts below -20 and bumps
 * `scale`/re-rolls `bright` from `mode` and a `rand()%5` jitter; then, if
 * `mode<26`, sets `alfa = mode*5` (else the 0x80 default), integrates
 * `pos += vec`, writes the sprite's position/scale/color/rotate, draws it,
 * and finally decrements `mode` (`+0xff`, i.e. a real `-1` wrap for the u8
 * field — see the encoding note below), disposing the slot when the OLD
 * mode was 0.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `param = &ef->param.smoke;` (SmokeType* at ef+4) — effect.h's proven
 *    layout (`vec`@0, `pos`@8, `rotate`@0x18, `scale`@0x1c, `mode`@0x20,
 *    `bright`@0x21, `unk22`@0x22), no new struct.
 *  - `sprSmoke` is genuinely `Sprite3D *sprSmoke[]` (unknown size, an
 *    array of pointers) — despite PSX.SYM's single-pointer prototype, the
 *    target indexes it (`sll idx,2; addu base,idx`) before the one `lw`
 *    that yields `spr`.
 *  - `Sprite3D` isn't in effect.h; declared locally in full (140 bytes),
 *    same TU-local-shadow convention as DrawHinoko.c (identical layout,
 *    copied verbatim — GsCOORDINATE2.coord.t[0..2] land at +0x18/+0x1c/
 *    +0x20 off the Sprite3D pointer, `sprite.rotate` at +0x88).
 *  - `param->vec.vx = (param->vec.vx * 80) / 100;` and the `vz` twin are
 *    plain constant-divisor arithmetic — cc1's own magic-multiply
 *    expansion (`0x51EB851F`, the standard divide-by-100 constant)
 *    reproduces the target exactly, no manual bit-tricks needed.
 *  - `param->vec.vy = param->vec.vy / 2;` (a genuine signed `/2`) likewise
 *    self-expands to the sign-correct-then-shift sequence automatically.
 *  - `vz`'s OLD value must be captured into a temp BEFORE `vec.vy`'s own
 *    update statement, with the `vec.vz` WRITE-BACK statement written
 *    LAST (after `vec.vy`'s) — the target's store order is `vx, vy, vz`
 *    even though `vz`'s value is loaded (for the /100) right after `vx`'s
 *    own store, i.e. loaded early, stored late (Ghidra's own `sVar1`
 *    capture confirms the split): `vx_new = ...; vz_old = param->vec.vz;
 *    param->vec.vy = param->vec.vy / 2; param->vec.vz = (vz_old*80)/100;`
 *  - `param->bright = (param->mode - 1) - (rand() % 5);` reassociates under
 *    fold into `mode - (remainder + 1)` (the WRONG shape — target keeps
 *    `mode-1` as its own subtraction) no matter how it's parenthesized;
 *    already documented for the sibling SetSmoke.c: "fold reassociates
 *    `(x-1)-(a+b)` into `x-(a+b+1)`, so an own-statement temp (`m =
 *    smoke->mode - 1;`) is needed to keep the literal `addiu -1` on x's
 *    side." Matching SetSmoke's exact idiom: `r = rand(); m = param->mode -
 *    1; param->bright = m - r % 5;` — THREE statements in that order (the
 *    bare `rand()` call first, `mode-1` second so it loads/subtracts
 *    AFTER the call returns instead of needing to survive across it, the
 *    final combine third) is load-bearing; folding the `% 5` into the
 *    `rand()` statement (`r = rand() % 5;`) reintroduces 4 stray bytes.
 *  - `spr = *sprp;` through an explicit `Sprite3D **sprp = &sprSmoke[idx];`
 *    pointer-to-pointer local — not a direct `spr = sprSmoke[idx];` — is
 *    the permuter-found fix for a final 8-byte prologue callee-saved-
 *    register SAVE ORDER tie (`sw s0` vs `sw s2` swapped): the extra
 *    indirection shifts allocation priority enough to match the target's
 *    order. Bisected from the permuter's raw winner, which also carried an
 *    inert `if ((!param) && (!param)) {}` — dead noise, confirmed by
 *    dropping it with no byte change.
 *  - `rotate = param->rotate;` is captured into its own temp, read right
 *    after `spr->scale = param->scale;` but STORED only after the r/g/b
 *    writes (`spr->sprite.rotate = rotate;`) — Ghidra's own `lVar4 =
 *    ...rotate; ...; sprite.rotate = lVar4;` capture confirms the split;
 *    without it the r/g/b stores drift one instruction early, filling a
 *    load-delay slot the target leaves as a bare `nop`.
 *  - `if (param->mode < 26) { alfa = param->mode * 5; }` re-reads `mode`
 *    TWICE (compare, then the multiply) rather than caching it — a plain
 *    `if` with the field used in both the condition and body naturally
 *    does this; `alfa` defaults to 0x80 (materialized once, at the very
 *    top, in the `mode==bright` guard's own delay slot).
 *  - `spr->locate.coord.t[0..2]` re-read `param->pos.vx/vy/vz` FRESH from
 *    memory right after storing them (matching DrawHinoko's own "final
 *    sprite-field stores re-read pos.* fresh" note) — direct field reads,
 *    not reused locals.
 *  - The disposal decrement needs its own captured `oldmode` local, read
 *    ONCE and used for BOTH the `+0xff` store and the `==0` test — the
 *    target's single `lbu` feeds both `addiu v1,v0,0xff` (new value,
 *    stored unconditionally) and `andi v0,v0,0xff` (the old value,
 *    re-tested) from the SAME load: `u8 oldmode = param->mode; param->mode
 *    = param->mode + 0xff; if (oldmode == 0) { ef->proc = 0; }` — verified
 *    a real `+0xff` (0x00FF, positive) here, unlike DrawBleed's `time-1`.
 */
typedef struct
{
    GsCOORDINATE2 locate; /* +0x00 */
    SVECTOR rotate;       /* +0x50 */
    s16 id;               /* +0x58 */
    s16 attribute;        /* +0x5a */
    SVECTOR clip;         /* +0x5c */
    long scale;           /* +0x64 */
    GsSPRITE sprite;      /* +0x68 */
} Sprite3D;

extern Sprite3D *sprSmoke[];
extern void UpdateCoordinate(Sprite3D *m);
extern void DrawSprite(Sprite3D *s);

void DrawSmoke(TEffectSlot *ef)
{
    SmokeType *param = &ef->param.smoke;
    Sprite3D *spr;
    Sprite3D **sprp;
    u8 alfa;
    u8 oldmode;
    s32 vz_old;
    s32 r;
    s32 m;
    s32 rotate;

    sprp = &sprSmoke[param->unk22];
    spr = *sprp;
    alfa = 0x80;

    if (param->mode == param->bright)
    {
        if (param->vec.vy < -20)
        {
            param->vec.vx = (param->vec.vx * 80) / 100;
            vz_old = param->vec.vz;
            param->vec.vy = param->vec.vy / 2;
            param->vec.vz = (vz_old * 80) / 100;
        }
        param->scale = param->scale + 0x400;
        r = rand();
        m = param->mode - 1;
        param->bright = m - r % 5;
    }

    if (param->mode < 26)
    {
        alfa = param->mode * 5;
    }

    param->pos.vx += param->vec.vx;
    param->pos.vy += param->vec.vy;
    param->pos.vz += param->vec.vz;
    spr->locate.coord.t[0] = param->pos.vx;
    spr->locate.coord.t[1] = param->pos.vy;
    spr->locate.coord.t[2] = param->pos.vz;
    spr->scale = param->scale;
    rotate = param->rotate;
    spr->sprite.r = alfa;
    spr->sprite.g = alfa;
    spr->sprite.b = alfa;
    spr->sprite.rotate = rotate;
    UpdateCoordinate(spr);
    DrawSprite(spr);

    oldmode = param->mode;
    param->mode = param->mode + 0xff;
    if (oldmode == 0)
    {
        ef->proc = 0;
    }
}
