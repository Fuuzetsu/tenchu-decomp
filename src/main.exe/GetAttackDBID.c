#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetAttackDBID(struct Humanoid *human, short mid);
 *     APPEAR.C:258, 8 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct Humanoid * human
 *     param $a1       short mid
 *
 * Globals it touches, as the original declared them:
 *     extern struct BattleType BattleDB[78];
 * END PSX.SYM */

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
