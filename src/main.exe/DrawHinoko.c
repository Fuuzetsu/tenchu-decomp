#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawHinoko(struct tag_EffectSlot *ef);
 *     EFFECT.C:1258, 61 src lines, frame 32 bytes, saved-reg mask 0x80010000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a1       struct tag_EffectSlot * ef
 *     reg   $a2       struct ExplosionType * param
 *     reg   $s0       struct Sprite3D * spr
 *     reg   $a3       unsigned char alfa
 *
 * Globals it touches, as the original declared them:
 *     extern struct Sprite3D *sprBomb[3];
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 67 of 308 bytes differ; the function is 1
 * instruction short (76 of the target's 77) — see the residual note at the
 * bottom of this comment.
 *
 * DrawHinoko (0x800368b0, 0x134 bytes) — bomb/explosion sprite proc:
 * bumps the explosion state machine (mode 0 -> 1 with a 30-tick timer;
 * mode 1 fades an alpha value from the countdown and self-disposes at 0),
 * then integrates the SVECTOR velocity into the VECTOR position every
 * frame, applies it to sprBomb[2], and draws it.
 *
 * Matching notes:
 *  - `(ef->param)` here is `struct ExplosionType`, NOT `BloodType` — a
 *    DIFFERENT field layout occupying the same union bytes (effect.h's new
 *    `explosion` member): `vec`(SVECTOR)@0x0, `pos`(VECTOR)@0x8,
 *    `rotate`@0x18, `scale`@0x1c, `time`@0x20, `mode`@0x21. Ghidra's names
 *    ("blood.py"/"blood.pz"/"blood.scale"/"smoke.*") are its own union
 *    disambiguation guesses and are WRONG about which member is live here;
 *    only the numeric offsets (confirmed against the raw `.s`) are trusted.
 *  - The `alfa` (0x80 default -> countdown-derived fade) and the
 *    `ef->proc = 0` self-dispose are NOT a single `&&`-chained condition
 *    despite Ghidra's rendering (`(mode==1) && (t=..., alfa=f(t), t==0)`):
 *    the real asm computes `alfa` UNCONDITIONALLY once inside the
 *    `mode == 1` arm (its `srl` sits in a branch delay slot that always
 *    executes) and ONLY THEN separately tests `t == 0` for the dispose —
 *    two statements, not a comma-chained guard.
 *  - `param->vec.vy` is read TWICE with different widths for different
 *    purposes 2 instructions apart, un-CSE'd: the narrowing re-store
 *    (`vec.vy = vec.vy + 5`) loads `lhu` (result truncates back to 16
 *    bits), the later full-width add into `pos.vy` loads `lh` (needs the
 *    sign) — give the narrowing use its own short temp so both loads fall
 *    out naturally (cookbook's DeleteConflict rule).
 *  - The final sprite-field stores (`spr->locate.coord.t[0..2]`,
 *    `spr->scale`) re-read `param->pos.*`/`param->scale` FRESH from memory
 *    (matching Ghidra's literal rendering) instead of reusing the values
 *    just computed above — write them as direct field reads, not cached
 *    locals.
 *
 * RESIDUAL (1 instruction / most of the 67 differing bytes): the target's
 * SCHEDULER hoists the `scale` field's READ far earlier (right after the
 * `time` read, before vx/vz) while its WRITE (+0xc00) stays at its source
 * position (after the `vy` read) — a read/write split entirely inside ONE
 * compound `param->scale = param->scale + 0xc00;` statement. The same
 * split happens for `rotate` at the tail (read right after `scale`'s
 * store, but the STORE into `spr->sprite.rotate` is deferred into the
 * `UpdateCoordinate` call's delay slot, after the r/g/b stores). This
 * draft's compiled schedule keeps each pair adjacent instead. Tried:
 * reordering the `scale`/`vy`/`pos.vz` statements in every relative order;
 * `tools/autorules.py` (found only an unsound `pz: long->u16` narrowing —
 * rejected, it turns the target's plain `lw` into `lhu`+`andi`, wrong
 * width for a proven `long` VECTOR field, cookbook's "reject an autorules
 * win that changes a proven field's access width"); one bounded permuter
 * run (~5 min, `-j4 --stop-on-zero`, see PID in this session). This is a
 * pure scheduling tie (right instruction SET, wrong SLOT), not a
 * structural error.
 */
/* Sprite3D isn't in effect.h; item.h has one but truncated at `scale`
 * (safe for its own TU, which never touches the trailing GsSPRITE sub-
 * struct DrawHinoko needs) — declared locally, full 140-byte layout, per
 * DrawEffect.c's precedent of TU-local type decls even where a same-named,
 * differently-scoped type exists elsewhere. */
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

extern Sprite3D *sprBomb[3];
extern void UpdateCoordinate(Sprite3D *m);
extern void DrawSprite(Sprite3D *s);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawHinoko", DrawHinoko);
#else /* NON_MATCHING */

static void DrawHinoko(TEffectSlot *ef)
{
    ExplosionType *param;
    Sprite3D *spr;
    u8 alfa;
    u8 t;
    long vz, vx;
    s16 vy;
    long vy2;
    long pz;

    param = &ef->param.explosion;
    spr = sprBomb[2];
    alfa = 0x80;
    if (param->mode == 0) goto mode0;
    if (param->mode == 1) goto mode1;
    goto join;
mode0:
    if (param->time == 0)
    {
        param->mode = 1;
        param->time = 0x1e;
    }
    goto join;
mode1:
    t = param->time;
    alfa = (u8)((t * 0x80) / 30);
    if (t == 0)
    {
        ef->proc = 0;
    }
join:

    vx = param->vec.vx;
    vz = param->vec.vz;
    param->time = param->time - 1;
    vy = param->vec.vy;
    param->scale = param->scale + 0xc00;
    pz = param->pos.vz;
    param->vec.vy = vy + 5;
    param->pos.vz = pz + vz;
    vy2 = param->vec.vy;
    param->pos.vx = param->pos.vx + vx;
    param->pos.vy = param->pos.vy + vy2;
    spr->locate.coord.t[0] = param->pos.vx;
    spr->locate.coord.t[1] = param->pos.vy;
    spr->locate.coord.t[2] = param->pos.vz;
    spr->scale = param->scale;
    spr->sprite.r = alfa;
    spr->sprite.g = alfa;
    spr->sprite.b = alfa;
    spr->sprite.rotate = param->rotate;
    UpdateCoordinate(spr);
    DrawSprite(spr);
}

#endif /* NON_MATCHING */
