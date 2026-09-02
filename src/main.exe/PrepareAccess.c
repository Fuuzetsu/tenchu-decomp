#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PrepareAccess(void);
 *     FILEIO.C:144, 8 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern int AccessPower;
 * END PSX.SYM */

extern void VSyncCallback(void (*f)(void));
extern void cbAccess(void);

void PrepareAccess(void)
{
    if (AccessPower >= 0)
    {
        AccessPower = 0;
        VSyncCallback(cbAccess);
    }
    else
    {
        VSyncCallback(0);
    }
}
