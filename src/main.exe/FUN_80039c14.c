#include "common.h"
#include "main.exe.h"

/*
 * FUN_80039c14 (0x80039c14, 0x40 bytes) — resets every slot of the
 * EffectSlot[] pool (200 entries, tag_EffectSlot from the Ghidra type
 * export) whose `proc` isn't the sentinel handler FUN_80032610, clearing it
 * to NULL. `proc` is the only field this function ever touches; the rest of
 * tag_EffectSlot (a `_180fake_1` union in the Ghidra export) is kept as
 * padding — EffectSlot's neighbor symbol (ModelSlot @ 0x8008dbe0) sits
 * exactly 200*0x4C bytes after EffectSlot, confirming the 0x4C stride/200
 * count used here.
 *
 * A plain `for` loop's entry test (i=0 < 200) is provably true and folds
 * away (cookbook Loops/leResetEnemyLayout), leaving the textbook
 * bottom-test-only do-while shape; EffectSlot[i] strength-reduces to a
 * walking pointer automatically (same "write the indexed form" precedent as
 * is_character_state_present_on_stage_'s HumanGroup[i]).
 */

extern void FUN_80032610(void);

typedef struct
{
    void (*proc)(void);
    u8 pad[0x48];
} tag_EffectSlot; /* 0x4C */

extern tag_EffectSlot EffectSlot[0xC8];

void FUN_80039c14(void)
{
    s32 i;

    for (i = 0; i < 0xC8; i++)
    {
        if (EffectSlot[i].proc != FUN_80032610)
        {
            EffectSlot[i].proc = 0;
        }
    }
}
