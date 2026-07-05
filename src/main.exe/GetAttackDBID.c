#include "common.h"
#include "main.exe.h"

/*
 * GetAttackDBID (0x8002a8ec, 0x8c bytes) — resolves the character's current
 * motion to a row index into the BattleDB attack-pattern table: linear-search
 * BattleDB[].mid for a match against GetMotionID(character's
 * something_about_current_animation, mid), sentinel-terminated by
 * mid == -1. Returns the matching row's index, or (no match before the
 * sentinel) the sentinel's own index.
 *
 * Plain `while (BattleDB[i].mid != -1) { if (match) break; i++; }` — jump.c's
 * duplicate_loop_exit_test copies the condition to the entry using the
 * provable i==0 (a literal offset-0 access, no index register), and again at
 * the loop bottom with the updated i (see the cookbook's bottom-test
 * do-while shape). `i` is s16: cc1 folds its sign-extend-then-scale-by-
 * sizeof(BattleDBEntry)==0x10 into one `sll 16/sra 12` pair instead of a
 * separate extend + `sll 4`.
 */

typedef struct
{
    s16 mid;
    u8 unk2[0xE];
} BattleDBEntry;

extern BattleDBEntry BattleDB[];
extern s16 GetMotionID(something_about_current_animation *motion, s16 mid);

s16 GetAttackDBID(character_state *human, s16 mid)
{
    s16 target;
    s16 i;

    target = GetMotionID(human->something_about_current_animation, mid);
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
