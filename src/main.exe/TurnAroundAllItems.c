#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void TurnAroundAllItems(struct Humanoid *user);
 *     ITEM.C:4023, 10 src lines, frame 88 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s5       struct Humanoid * user
 *     reg   $s4       int i
 *     reg   $s1       int j
 *     reg   $s5       struct Humanoid * human
 *     reg   $s4       int itemID
 *     stack sp+16     struct PARAM_ITEM_LAUNCH p
 * END PSX.SYM */

#include "item.h"

/*
 * Drop every carried item at the user's root-model position, giving each copy
 * a small random launch vector, then clear the inventory counts.
 *
 * Matching notes:
 *  - The two top-tested `while (1)` loops preserve the target's explicit
 *    counter tests and unconditional backedges.
 *  - `human` and `itemID` are distinct block-local captures.  Together with
 *    the stack PARAM_ITEM_LAUNCH object, they reproduce the original saved-
 *    register allocation and the call setup for ReqItemDrop.
 *  - Keep each rand call inline in its modulo expression so its result stays
 *    in $v0 and the three magic-division sequences retain their target shape.
 */
void TurnAroundAllItems(Humanoid *user)
{
    s32 i;
    s32 j;
    PARAM_ITEM_LAUNCH p;

    i = 0;
    while (1)
    {
        if (i >= ITEM_N)
            break;
        j = 0;
        while (1)
        {
            VECTOR *pos;
            Humanoid *human;
            s32 itemID;

            if (j >= user->item[i])
                break;
            pos = GetAbsolutePosition(user->model->object[MODEL_PART_WAIST], 0, 0, 0);
            human = user;
            itemID = i;
            memset(&p, 0, sizeof(p));
            p.type = itemID;
            p.user = human;
            p.start.vx = pos->vx;
            p.start.vy = pos->vy;
            p.start.vz = pos->vz;
            p.end.vx = rand() % 200 - 100;
            p.end.vy = rand() % 100 - 200;
            p.end.vz = rand() % 200 - 100;
            ReqItemDrop(&p);
            j++;
        }
        user->item[i] = 0;
        i++;
    }
}
