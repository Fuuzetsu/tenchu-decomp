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
 * GotoPosition extern from main.exe.h).
 *
 * The mask is semantically the PADLleft|PADLright turn-only filter used by
 * Think1sleep/AttackAnimal/StateTransition, but signedness picks the
 * instruction shape (fits-andi lever). Their unsigned 0xA000 value compiles
 * to `andi`; PAD_TURN_BUTTONS_SIGNED instead materializes 0xFFFFA000 with
 * `addiu` and uses a register-register `and`, matching this target.
 */

s16 Think2confirm(void)
{
    return GotoPosition(0, 0) & PAD_TURN_BUTTONS_SIGNED;
}
