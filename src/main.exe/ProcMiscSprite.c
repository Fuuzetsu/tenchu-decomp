#include "common.h"
#include "main.exe.h"
#include "misc.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcMiscSprite(struct tag_TMisc *m, enum TMiscMessage msg);
 *     MISC.C:582, 50 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_TMisc * m
 *     param $a1       enum TMiscMessage msg
 *     reg   $v1       int type
 *     reg   $s0       struct Sprite3D * s
 * END PSX.SYM */

/*
 * ProcMiscSprite (0x8004d874, 0xFC bytes across two pieces — the second, a
 * Ghidra `__override__prt_` call-site marker for the AdtMessageBox variadic
 * call, is NOT a jump table: piece 1 falls straight through into it, no
 * branch targets it (cookbook: "__override__prt isn't always a jump
 * table")). MISC_SPRITE's ProcMisc* handler: MM_CREATE clamps the raw
 * `param.init.a` down to a valid SpriteData index and re-stores it as the
 * TSprite.type byte at the SAME union offset (0x18); any message
 * at least MM_DO (the "draw" tick) recolors the sprite a random grey and
 * copies m's position into it.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `msg >= MM_DO` compiles UNSIGNED (`sltiu`, confirmed against the raw
 *    .s alongside ProcMiscFire's identical dispatch prologue) — declaring
 *    `msg` as the actual `enum TMiscMessage` parameter (all-non-negative
 *    enumerators) is what gets cc1 to pick the unsigned compare; a plain
 *    `s32 msg` would emit `slti` instead (wrong instruction, though same
 *    length).
 *  - The union's two views at offset 0x18: `m->param.init.a` (s32) for the
 *    raw CREATE read, `m->param.sprite.type` (u8) for the clamped
 *    store-back and every later read — reproduces the lw-then-sb/lbu width
 *    switch at the identical address (cookbook: a param-union store whose
 *    access WIDTH differs from another view at the same offset is a
 *    distinct union member).
 *  - `rand() % 60 + 0x62` is one expression — the call stays inline so the
 *    magic-multiply divide operates directly on $v0 (cookbook: keep calls
 *    inline in expressions). The `b = g = r = value` chain evaluates its
 *    stores as r, g, b, reproducing retail without a source-level grey
 *    carrier.
 */

extern short DrawSprite(Sprite3D *sprt);
/* Shared MISC.C rodata pool (same block as AddMisc.c's path_image_2/fmt_undefined_effect
 * strings, carved via AddMisc's .rodata segment) — "unknown sprite type",
 * NOT a fresh literal in this file's own rodata (which would place it at
 * the wrong address, cookbook: TU-shared string pooling). */
extern char msg_unknown_sprite_type[]; /* unknown sprite type */

void ProcMiscSprite(TMisc *m, TMiscMessage msg)
{
    s32 type;
    Sprite3D *s;

    if (msg == MM_CREATE)
        goto do_create;
    if (msg >= MM_DO)
        goto do_draw;
    return;

do_create:
    type = m->param.init.a;
    if (type >= N_MISC_SPRITE_TYPES)
    {
        AdtMessageBox(msg_unknown_sprite_type);
        type = MISC_SPRITE_FIRE1;
    }
    m->mode = 0;
    m->param.sprite.type = (misc_sprite_kind)type;
    return;

do_draw:
    s = SpriteData[m->param.sprite.type].spr;
    s->sprite.b = s->sprite.g = s->sprite.r = (u8)(rand() % 60 + 0x62);
    s->locate.coord.t[0] = m->x;
    s->locate.coord.t[1] = m->y;
    s->locate.coord.t[2] = m->z;
    UpdateCoordinate((ModelType *)s);
    DrawSprite(s);
}
