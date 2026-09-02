#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1watch(void);
 *     THINK_1.C:50, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a0       long xx
 *     reg   $a1       long zz
 *     reg   $s0       short pad
 *     reg   $a0       long vx
 *     reg   $a1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 * END PSX.SYM */

extern int rand(void);

s16 Think1watch(void)
{
    s16 pad;

    UPDATE_IDLE_LOOK_PAD(pad);
    return pad;
}
