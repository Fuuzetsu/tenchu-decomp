#include "common.h"
#include "main.exe.h"
#include "misc.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcMiscFire(struct tag_TMisc *m, enum TMiscMessage msg);
 *     MISC.C:249, 41 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct tag_TMisc * m
 *     param $a1       enum TMiscMessage msg
 *     stack sp+16     struct SVECTOR vec
 *     stack sp+24     struct VECTOR pos
 * END PSX.SYM */

extern SVECTOR svec_y_n35[];

void ProcMiscFire(TMisc *m, TMiscMessage msg)
{
    SVECTOR vec;
    VECTOR pos;

    if (msg == MM_CREATE)
        goto do_create;
    if (MM_DO <= msg)
        goto do_check;
    return;

do_create:
    m->mode = 0;
    m->count = 10;
    return;

do_check:
    if (m->mode != 0)
        return;
    m->count--;
    if (m->count < 1)
    {
        vec = svec_y_n35[0];
        pos.vx = m->x;
        pos.vy = m->y;
        pos.vz = m->z;
        SetExplosion(&pos, &vec);
        vec.vx = 75;
        vec.vy = 180;
        vec.vz = 75;
        SetHinoko(&pos, &vec, 10);
        vec.vx = 0;
        vec.vy = -200;
        vec.vz = 0;
        SetSmoke(&pos, &vec, 20, 6);
        m->count = rand() % 150;
        SoundEx(&pos, SE_FIRE);
    }
}
