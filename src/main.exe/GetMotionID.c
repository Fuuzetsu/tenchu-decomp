#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetMotionID(struct MotionManager *mmp, short mid);
 *     ACTION.C:169, 9 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct MotionManager * mmp
 *     param $a1       short mid
 * END PSX.SYM */

/*
 * GetMotionID (0x8001c510, 0x74 bytes) — search mmp's registered-motion
 * table (MotionRegistType[], sentinel mid == -1) for the row whose `mid`
 * matches the requested id and return that row's `id`; on falling through to
 * the sentinel without a match, returns the sentinel row's own `id` instead.
 * Same search-with-break-then-read-index shape as GetAttackDBID.c (a near-
 * twin, which itself calls this to resolve the character's current motion
 * before its own BattleDB search) — just reading a struct field instead of
 * the loop index at the end.
 */

s16 GetMotionID(MotionManager *mmp, s16 mid)
{
    MotionRegistType *reg;
    s16 i;

    reg = mmp->motreg;
    i = 0;
    while (reg[i].mid != -1)
    {
        if (reg[i].mid == mid)
        {
            break;
        }
        i++;
    }
    return reg[i].id;
}
