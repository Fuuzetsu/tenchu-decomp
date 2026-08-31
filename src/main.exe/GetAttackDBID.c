#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetAttackDBID(struct Humanoid *human, short mid);
 *     APPEAR.C:258, 8 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short mid
 *
 * Globals it touches, as the original declared them:
 *     extern struct BattleType BattleDB[78];
 * END PSX.SYM */

/*
 * GetAttackDBID (0x8002a8ec, 0x8c bytes) — resolves the character's current
 * motion to a row index into the BattleDB attack-pattern table: linear-search
 * BattleDB[].mid for a match against GetMotionID(human->motion, mid), sentinel-terminated by
 * mid == -1. Returns the matching row's index, or (no match before the
 * sentinel) the sentinel's own index.
 *
 * Plain `while (BattleDB[i].mid != -1) { if (match) break; i++; }` — jump.c's
 * duplicate_loop_exit_test copies the condition to the entry using the
 * provable i==0 (a literal offset-0 access, no index register), and again at
 * the loop bottom with the updated i (see the cookbook's bottom-test
 * do-while shape). `i` is s16: cc1 folds its sign-extend-then-scale-by-
 * sizeof(BattleType)==0x10 into one `sll 16/sra 12` pair instead of a
 * separate extend + `sll 4`.
 */

s16 GetAttackDBID(Humanoid *human, s16 mid)
{
    s16 target;
    s16 i;

    target = GetMotionID(human->motion, mid);
    i = 0;
    while (BattleDB[i].mid != -1)
    {
        if (BattleDB[i].mid == target)
        {
            break;
        }
        i++;
    }
    return i;
}
