#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think2confirm(void);
 *     THINK_2.C:14, frame 16 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 * END PSX.SYM */

/*
 * Think2confirm (0x8002fa24, 0x30 bytes) — think-handler, same "think" TU as
 * Think1sleep.c/ThinkBasicHuman2.c (s16 return convention; shared
 * turn_towards_player_ extern from main.exe.h).
 *
 * The mask literal's SPELLING picks the instruction shape (fits-andi lever):
 * Think1sleep.c's `uVar1 & 0xA000` (a positive literal, fits the unsigned
 * 16-bit andi immediate) compiles to `andi $a1,$v0,0xa000`. Here the asm
 * instead materializes a full register constant (`addiu $v1,$zero,-0x6000`)
 * and does a register-register `and` — the target's literal does NOT fit
 * andi's unsigned-16-bit test, i.e. it's negative: `~0x5FFF` (bitwise NOT of
 * a positive 16-bit constant) is exactly -0x6000/0xFFFFA000, confirmed
 * against the encoded `addiu` immediate.
 */

s16 Think2confirm(void)
{
    return turn_towards_player_(0, 0) & ~0x5FFF;
}
