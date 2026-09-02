#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "humanoid.h"
#include "game_globals.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1trace(void);
 *     THINK_1.C:16, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a0       long xx
 *     reg   $a1       long zz
 *     reg   $t0       short pad
 *     reg   $a0       long vx
 *     reg   $a1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Degree;
 *     extern short Attrib;
 * END PSX.SYM */

extern Humanoid *Me_THINK_C;

s16 Think1trace(void)
{
    s16 pad;

    pad = 0;
    if (Me_THINK_C->actcnt == 0)
    {
        u8 old_actscnt;

        old_actscnt = Me_THINK_C->actscnt;
        Me_THINK_C->actscnt = old_actscnt + 1;
        if (old_actscnt < 60)
        {
            Humanoid *self;
            s32 turn;
            s32 degree;
            s32 abs_degree;

            if (Me_THINK_C->actcnt || degree)
            {
                self = Me_THINK_C;
                degree = Degree;
                turn = self->turn;
                abs_degree = degree;
            }
            else
            {
                self = Me_THINK_C;
                degree = Degree;
                turn = self->turn;
                abs_degree = degree;
            }
            if (degree < 0)
            {
                abs_degree = -abs_degree;
            }
            if (turn < abs_degree)
            {
                if (self->actscnt < 30)
                {
                    pad = -PADLleft;
                    if (turn < degree)
                    {
                        pad = PADLright;
                    }
                }
            }
        }
        else
        {
            Me_THINK_C->actcnt = 1;
            Me_THINK_C->actscnt = 0;
        }
    }
    else
    {
        Me_THINK_C->actcnt++;
        if (Attrib & ATTR_TRACE)
        {
            pad = ControlTraceLine(Me_THINK_C);
        }
    }
    return pad;
}
