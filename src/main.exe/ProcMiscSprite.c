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

extern short DrawSprite(Sprite3D *sprt);
extern char msg_unknown_sprite_type[]; /* unknown sprite type */

void ProcMiscSprite(TMisc *m, TMiscMessage msg)
{
    s32 type;
    Sprite3D *s;

    switch (msg)
    {
    case MM_CREATE:
        type = m->param.init.a;
        if (type >= N_MISC_SPRITE_TYPES)
        {
            AdtMessageBox(msg_unknown_sprite_type);
            type = MISC_SPRITE_FIRE1;
        }
        m->mode = 0;
        m->param.sprite.type = (misc_sprite_kind)type;
        break;

    case MM_DESTROY:
    case MM_PAUSE:
    case MM_RESUME:
        break;

    default:
        s = SpriteData[m->param.sprite.type].spr;
        s->sprite.b = s->sprite.g = s->sprite.r =
            (u8)(rand() % 60 + 0x62);
        s->locate.coord.t[0] = m->x;
        s->locate.coord.t[1] = m->y;
        s->locate.coord.t[2] = m->z;
        UpdateCoordinate((ModelType *)s);
        DrawSprite(s);
        break;
    }
}
