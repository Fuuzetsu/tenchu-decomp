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

/*
 * ThinkBasicHuman1 (0x8002f820, 0xa4 bytes) — think-handler (same "think" TU
 * as Think1sleep.c/Think1trace.c/ThinkBasicHuman2.c): reads port-0 held
 * buttons through remap_buttons_ (a pad-processing helper, not yet named/
 * matched), clears them all if bit 0x100 is held while `SYSFLAG_DEBUGPRINT`
 * is set (a cheat/debug toggle), masks to the low 12 bits while the area
 * attribute bit 0x200 is set and the character is jumping or attacking
 * (STAT_JUMP 9 / STAT_ATTACK 7), then remaps bit 3 to bit 5 (0x8 -> 0x20).
 *
 * `Me_THINK_C->map.attrib` (game_types.h, field @0x28 — see its header
 * comment there) is proven by the raw `lhu` here; status is the shared
 * signed Humanoid field.
 *
 * remap_buttons_'s return must be a WIDE type (s32, not s16) in this caller: no
 * sll/sra re-extension follows its call (unlike GetPad's result, which DOES
 * get the short-result pair right before being passed in as the argument) —
 * the "Ghidra's short-typed call-result variable can be int in source" rule.
 */
extern s32 remap_buttons_(s16 pad);

s16 ThinkBasicHuman1(void)
{
    s32 pad;

    pad = remap_buttons_(GetPad(0));
    if ((pad & PADselect) && (SystemFlag & SYSFLAG_DEBUGPRINT))
    {
        pad = 0;
    }
    if ((Me_THINK_C->map.attrib & MAP_DEATH) &&
        (Me_THINK_C->status == STAT_JUMP || Me_THINK_C->status == STAT_ATTACK))
    {
        pad &= 0xfff;
    }
    if (pad & PADR1)
    {
        pad = (pad & (PADLleft | PADLdown | PADLright | PADLup | PADstart | PADj | PADi | PADselect | PADRleft | PADRdown | PADRright | PADRup | PADL1 | PADR2 | PADL2)) | PADRright;
    }
    return pad;
}
