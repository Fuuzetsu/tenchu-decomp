#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "padcmd.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ThinkBasicHuman1(void);
 *     THINK.C:236, 7 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern enum TSystemFlag SystemFlag;
 * END PSX.SYM */

extern s32 remap_buttons_(s16 pad);

s16 ThinkBasicHuman1(void)
{
    s32 pad;

    pad = remap_buttons_(GetPad(PAD_CONTROLLER_1));
    if ((pad & PADselect) && (SystemFlag & SYSFLAG_DEBUGPRINT))
    {
        pad = 0;
    }
    if ((Me_THINK_C->map.attrib & MAP_DEATH) &&
        (Me_THINK_C->status == STAT_JUMP || Me_THINK_C->status == STAT_ATTACK))
    {
        /* Limit the complement to the 16-bit pad word. */
        pad &= (u16)~(PADLup | PADLdown | PADLleft | PADLright);
    }
    if (pad & PADR1)
    {
        /* Limit the complement to the 16-bit pad word. */
        pad = (pad & (u16)~PADR1) | PADRright;
    }
    return pad;
}
