#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

/*
 * FUN_80039c14 (0x80039c14, 0x40 bytes) — resets every slot of the
 * EffectSlot[] pool (200 entries) whose `proc` isn't the sentinel handler
 * UpdateTexScroll, clearing it to NULL. EffectSlot's neighbor symbol
 * (ModelSlot @ 0x8008dbe0) sits
 * exactly 200*0x4C bytes after EffectSlot, confirming the 0x4C stride/200
 * count used here.
 *
 * A plain `for` loop's entry test (i=0 < 200) is provably true and folds
 * away (cookbook Loops/leResetEnemyLayout), leaving the textbook
 * bottom-test-only do-while shape; EffectSlot[i] strength-reduces to a
 * walking pointer automatically (same "write the indexed form" precedent as
 * is_character_state_present_on_stage_'s HumanGroup[i]).
 */

void FUN_80039c14(void)
{
    s32 i;

    for (i = 0; i < 0xC8; i++)
    {
        if (EffectSlot[i].proc != UpdateTexScroll)
        {
            EffectSlot[i].proc = 0;
        }
    }
}
