#include "common.h"
#include "main.exe.h"

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
 * buttons through FUN_8001b2f4 (a pad-processing helper, not yet named/
 * matched), clears them all if bit 0x100 is held while SystemFlag's bit 0 is
 * set (a cheat/debug toggle), masks to the low 12 bits while the area
 * attribute bit 0x200 is set and the character is jumping/landing
 * (character_status 9 or 7), then remaps bit 3 to bit 5 (0x8 -> 0x20).
 *
 * `Me_THINK_C->attrib` (game_types.h, new field @0x28 — see its header
 * comment there) is proven by the raw `lhu` here; `(s16)Me_THINK_C->
 * character_status` follows Think1ninja.c's established per-TU load-width
 * cast (proven u16 field; this TU's asm reads it `lh`).
 *
 * FUN_8001b2f4's return must be a WIDE type (s32, not s16) in this caller: no
 * sll/sra re-extension follows its call (unlike GetPad's result, which DOES
 * get the short-result pair right before being passed in as the argument) —
 * the "Ghidra's short-typed call-result variable can be int in source" rule.
 */
extern s16 GetPad(s16 port);
extern s32 FUN_8001b2f4(s16 pad);
extern u32 SystemFlag;

s16 ThinkBasicHuman1(void)
{
    s32 pad;

    pad = FUN_8001b2f4(GetPad(0));
    if ((pad & 0x100) && (SystemFlag & 1)) {
        pad = 0;
    }
    if ((Me_THINK_C->attrib & 0x200) &&
        ((s16)Me_THINK_C->character_status == 9 || (s16)Me_THINK_C->character_status == 7)) {
        pad &= 0xfff;
    }
    if (pad & 8) {
        pad = (pad & 0xfff7) | 0x20;
    }
    return pad;
}
