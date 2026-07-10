#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3firstattack(void);
 *     THINK_3.C:209, frame 16 bytes, saved-reg mask 0x00000000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $a0       short pad
 *     reg   $v1       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern short Attrib;
 *     extern short Degree;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 28 of 260 bytes differ (right length: the residual
 * doesn't shift anything downstream; `tools/matchdiff.py` whole-image count
 * equals the function-local count).
 *
 * Think3firstattack (0x8002db08, 0x104 bytes) — think-handler, same TU as
 * Think3chase.c/Think1trace.c (s16 return convention; gp-relative
 * Distance/Degree/SR/Attrib/Me_THINK_C). Deliberately does NOT include
 * main.exe.h: this file reads Attrib as u16 (`lhu`/`sh`), conflicting with
 * main.exe.h's `s16 Attrib` — same per-file respelling as
 * think_setting_go_towards_player.c/turn_towards_player_.c.
 *
 * Forwards to turn_towards_player_(0, 0) for the base turn signal, clears SR
 * when close and not already in the "-2" state, flags Attrib bit 0x10 when
 * the character is a civilian (character_kind in 0x90..0x9F — CHONIN/JOCHU/
 * MUSUME/ZAININ/BIZENYA/NAKAI/MEKAKE, game_types.h's civilian run), then
 * derives `wclass` from `(s16)weapon_kind >> 4` — the same weapon-category
 * nibble Think3chase.c's AttackFunc dispatch uses. For ranged weapons
 * (wclass == 3: TEPPO/GUN/YUMI/... all 0x30-0x37) masks the turn signal to
 * just the 0xA000 turn bits (spelled `~0x5FFF`, a negative literal — fits
 * the andi immediate test the OTHER way, li+and not andi, per Think2confirm)
 * and, if badly misaimed (abs(Degree) > 100), returns early with just that
 * masked signal. Otherwise (any weapon, or a ranged one aimed well enough),
 * compares Distance against a per-class engagement threshold in `atkd2` and,
 * if within range, ORs in the "attack" bit (0x80) and flags Attrib 0x10
 * again. ALL BYTE-PROVEN except the residual below (confirmed by matchdiff
 * showing only 7 differing instructions, all register-color/scheduling).
 *
 * `atkd`/`atkd2` (two new adjacent globals at 0x800979e0/0x800979e8, right
 * after Attrib and before Projection — no existing symbol occupied this
 * range): Ghidra's own independent decompilation of the sibling
 * Think3attack (0x8002d0b0, unmatched) already names `atkd` and indexes
 * `atkd[wclass]` directly (wclass 0..3). THIS function's table access
 * resolves to the SAME numeric address atkd+8 would give, but is a
 * SEPARATE, freshly-declared 4-entry symbol, not `atkd[wclass+4]`: per the
 * cookbook's "nonzero offset off a declared symbol always materializes that
 * symbol's OWN address, offset applied at the access" rule, an `atkd[]`
 * indexing expression compiles to `lh $v1,8($v0)` (base = &atkd, +8 folded
 * into the LOAD's displacement) — confirmed empirically here — whereas the
 * target bakes +8 into the SYMBOL's OWN lui/addiu (`lh $v1,0($v0)`, base
 * already = 0x800979e8). Only a genuinely separate symbol at that exact
 * address reproduces the target's bytes; declaring `atkd2` fixed this
 * cleanly (dropped the diff from 115 to 28 bytes).
 *
 * `wclass` is a genuine s16 local (not s32): it re-derives its
 * sign-extension from scratch at the array-index use (a fresh `sll 16/sra
 * 15` computing `(s16)wclass << 1` in a later extended basic block) rather
 * than reusing the s32 value already sitting in the register from the
 * assignment — the "true short variable re-extends at each use past a
 * join" tell (same family as GetDirection/PauseProc's short-vs-int levers).
 *
 * `turn_towards_player_` is declared returning `int` here (not `s16`): the
 * asm's `result = call();` copy right after the `jal` is a bare `addu`, no
 * immediate sll/sra pair — the caller-side extern-return-type-is-an-
 * extension-position lever (Think1trace/BIS's GetRealPad; same as
 * think_setting_go_towards_player.c's identical local declaration).
 *
 * RESIDUAL (28 bytes / 7 instructions, all inside the `wclass == 3` block):
 * the target computes `masked = result & ~0x5FFF` into a register SEPARATE
 * from `result` (v1), leaves it RAW (untruncated) through the abs(Degree)
 * check, and only truncates to s16 ONCE, in the function's single shared
 * return tail (shared by both the early `return masked` and the normal
 * `return result` path) — two genuine scheduler nops appear (load-delay
 * after `lh Degree`, branch-delay after the sign check) because nothing
 * else is independently ready to fill them. Every draft tried here (this
 * exact two-variable shape; `result`/`masked` reassigned in place as one
 * variable à la Ghidra's literal rendering; `result`/`masked` narrowed to
 * u16/s32/s16 in all 4 combinations; reordering the `degree`/`masked`
 * statements; hoisting `result = masked` unconditionally before the
 * abs(Degree) check; declaring `masked` at function scope; swapping
 * `result`/`wclass` declaration order) either reproduces the WRONG mask
 * instruction (`andi` instead of `li`+`and` — needs `result` non-narrow), or
 * a LENGTH mismatch (the scheduler hoists the independent `lh Degree` load
 * earlier and fills both slots with the mask's `li`/`and`, eliminating the
 * two nops — matches when `result`/`masked` are s32), or (the best draft
 * kept below, `result` s16 + `masked` s32) the right LENGTH but with the
 * mask's truncation-to-s16 computed EAGERLY (right after the AND, into a
 * fresh register) instead of deferred to the shared tail — `wclass` also
 * lands in $a2 instead of the target's $a0. `tools/regalloc.py` shows the
 * expected copy-chains (p80->a1, i104 a0->v0) but nothing pinpointing a
 * specific lever beyond what was already tried. One bounded permuter run
 * (~2.5 min, --stop-on-zero -j4) never reached a favorable state — every
 * candidate is reported against a baseline of "230+ errors" that grows over
 * iterations (looks like a harness/scoring mismatch for this function, not
 * real progress; its best-scoring candidate substituted a nonsensical
 * `long long masked`, not a genuine fix) — did not pursue further per the
 * attempt-cap guidance. decomp.me (psyq4.3 preset) would be the next
 * arbiter if revisited.
 */
extern character_state *Me_THINK_C;
extern s32 Distance;
extern s16 Degree;
extern s16 SR;
extern u16 Attrib;
extern s16 atkd2[4];
extern int turn_towards_player_(int x_diff, int z_diff);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/Think3firstattack", Think3firstattack);
#else /* NON_MATCHING */

s16 Think3firstattack(void)
{
    s16 result;
    s16 wclass;
    s32 degree;

    result = turn_towards_player_(0, 0);
    if (Distance < 10000 && SR != -2)
    {
        SR = 0;
    }
    if ((Me_THINK_C->character_kind & 0xF0) == 0x90)
    {
        Attrib |= 0x10;
    }
    wclass = (s16)Me_THINK_C->weapon_kind >> 4;
    if (wclass == 3)
    {
        s32 masked;

        masked = result & ~0x5FFF;
        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree > 100)
        {
            return masked;
        }
        result = masked;
    }
    if (Distance < atkd2[wclass])
    {
        result |= 0x80;
        Attrib |= 0x10;
    }
    return result;
}

#endif /* NON_MATCHING */
