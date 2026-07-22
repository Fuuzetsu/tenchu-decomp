#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * is_character_state_present_on_stage_ (0x800294dc, 0x50 bytes) — linear
 * search of the live HumanGroup[] pointer array (length Humans) for `cs`;
 * returns whether it was found. Same "Humanoid control" TU as MoveHumanoid.c
 * (address-contiguous with GetHumanoid/MoveHumanoid/GetDirection/etc.).
 * Cross-TU callers (ProcItemDrop.c etc.) declare it `s32 f(Humanoid *h)` —
 * this TU's own view of the object is `Humanoid` (Me_THINK_C's type
 * in main.exe.h); the parameter is never dereferenced here (pure pointer
 * compare), so the type choice is cosmetic, but kept consistent with the
 * rest of this batch.
 *
 * A plain `for` loop's entry test (i=0 < Humans, i.e. Humans>0) is
 * duplicated to the top by jump.c's duplicate_loop_exit_test, and the
 * per-iteration test is rotated to the bottom — the textbook bottom-test
 * shape (cookbook Loops). loop.c hoists the loop-invariant `Humans` load
 * into the preheader ($a2, read once), so the loop bound compares against
 * the cached register every iteration; the post-loop `return i != Humans`
 * is a separate statement that re-reads the global fresh (no shared pseudo
 * survives the loop's multiple exits) — no explicit source-level cache
 * variable needed for this. HumanGroup[i] strength-reduces to a walking
 * pointer (+4 each iteration) automatically; write the indexed form.
 */
extern s16 Humans;

s32 is_character_state_present_on_stage_(Humanoid *cs)
{
    s32 i;

    for (i = 0; i < Humans; i++)
    {
        if (HumanGroup[i] == cs)
            break;
    }
    return i != Humans;
}
