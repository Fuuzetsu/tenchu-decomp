#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCdaStatus CdaStatus;
 * END PSX.SYM */

/* CD-audio status (Ghidra: TCdaStatus). Only voll/volr are touched here. */
void set_cda_volume_(u8 voll, u8 volr)
{
    CdaStatus.voll = voll;
    CdaStatus.volr = volr;
}
