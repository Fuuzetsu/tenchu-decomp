#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawBG(struct BackGround *bg);
 *     3DCTRL.C:686, 6 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct BackGround * bg
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/* Official libgs name: sits immediately before GsInitFixBg16 in the
 * same module order as the demo's GsSortFixBg32/GsInitFixBg32 pair
 * (the demo's DrawBG called the Bg32 variant; retail switched to
 * 16x16 cells). The 4-arg shape is the real implementation ABI. */
extern void GsSortFixBg16(BackGround *bg, u32 *work, GsOT *ot, u16 sz);

short DrawBG(BackGround *bg)
{
    if ((bg->attribute & MODEL_ATTR_HIDDEN) != 0)
    {
        return 0;
    }
    GsSortFixBg16(bg, bg->work, OTablePt, bg->sz);
    return 1;
}
