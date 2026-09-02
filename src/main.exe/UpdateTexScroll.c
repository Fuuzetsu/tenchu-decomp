#include "common.h"
#include "main.exe.h"
#include "effect.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateTexScroll(struct TexScroll *tscr);
 *     EFFECT.C:1884, 24 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct TexScroll * tscr
 *     reg   $s0       struct DR_MOVE * prim
 * END PSX.SYM */

/* Demo symbols use TexScroll *; retail callback ABI passes its TEffectSlot owner. */
void UpdateTexScroll(TEffectSlot *ef)
{
    TexScroll *tscr;
    DR_MOVE *prim;

    tscr = &ef->param.texscroll;
    tscr->px = (u32)(tscr->px + tscr->vx) %
               (u32)(tscr->image.w << TEXSCROLL_SUBPIXEL_BITS);
    tscr->py = (u32)(tscr->py + tscr->vy) %
               (u32)(tscr->image.h << TEXSCROLL_SUBPIXEL_BITS);

    tscr->image.x = tscr->sx + tscr->px / TEXSCROLL_SUBPIXEL_SCALE;
    tscr->image.y = tscr->sy + tscr->py / TEXSCROLL_SUBPIXEL_SCALE;

    prim = (DR_MOVE *)GsGetWorkBase();
    GsSetWorkBase(prim + 1);
    SetDrawMove(prim, &tscr->image, tscr->x, tscr->y);
    AddPrim(OTablePt->org, prim);
}
