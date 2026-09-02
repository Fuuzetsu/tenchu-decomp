#include "common.h"
#include "main.exe.h"
#include "misc.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ResetAllMisc(void);
 *     MISC.C:147, 11 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TMisc misc[200];
 * END PSX.SYM */

void ResetAllMisc(void)
{
    TMisc *p;
    s32 i;

    for (i = 0; i < MaxMisc; i++)
    {
        p = &misc[i];
        if (p->proc != 0)
        {
            p->proc(p, MM_DESTROY);
            p->proc = 0;
        }
    }
}
