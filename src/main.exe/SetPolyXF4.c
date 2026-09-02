#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetPolyXF4(struct POLY_XF4 *ply, short attrib);
 *     EFFECT.C:1770, 5 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct POLY_XF4 * ply
 *     param $a1       short attrib
 * END PSX.SYM */

void SetPolyXF4(POLY_XF4 *ply, short attrib)
{
    setPolyF4(&ply->ply);
    setSemiTrans(&ply->ply, 1);
    setlen(&ply->tpage, GPU_PACKET_LENGTH(DR_TPAGE));
    ply->tpage.code[0] = GPU_DRAWMODE_BLEND(attrib) | GPU_DRAWMODE_DITHER;
}
