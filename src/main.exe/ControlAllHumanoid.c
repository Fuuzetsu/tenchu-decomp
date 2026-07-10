#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ControlAllHumanoid(void);
 *     HUMAN.C:97, 6 src lines, frame 32 bytes, saved-reg mask 0x80030000
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 11 of 196 bytes differ (right length: the residual
 * doesn't shift anything downstream; matchdiff's diff stays localized).
 *
 * ControlAllHumanoid (0x800292a4, 0xc4 bytes) — same "Humanoid control" TU
 * as GetHumanoid.c/MoveHumanoid.c/GetMoveSpeed.c/GetTargetDistance.c
 * (HUMAN.C). Clears VISIBLE_ENEMIES_ (proven s16, DoInfoViewProc.c/
 * update_something_for_each_visible_enemy_.c), then walks the live
 * HumanGroup[]/Humans array (the same short-counter fused sign-extend+scale
 * loop shape as GetHumanoid), calling ControlHumanoid(human) on every entry
 * whose attribute bit 0x80 is clear — bracketing the call with
 * character_balma_around_main_routine_() (the area-map cursor save/restore
 * helper, HUMAN.C's own name for FUN_8001aba0, called TWICE) when
 * human->type == 0x85 (BALMA).
 *
 * `*(u16 *)&human->attribute` forces the `lhu` this TU's access uses against
 * item.h's proven-signed `s16 attribute` (same per-TU load-width divergence
 * as HumanActionControl.c's identical cast on the same field/offset).
 *
 * A separate redundant-load bug fixed first: writing `result` as `s16`
 * (matching the Ghidra-rendered `sVar1`) made cc1 emit TWO loads of Humans
 * (an extra dead `lhu` into an unused register right at entry, before the
 * real signed `lh`) — the HImode store vs SImode compare expansions of the
 * same global apparently defeat cc1's CSE here. Declaring `result` as `s32`
 * instead collapses both references back onto ONE load, matching target.
 *
 * RESIDUAL (11 bytes): `result` (Humans's entry value, returned unchanged
 * when the loop never runs, else overwritten with 0 every iteration and
 * returned as 0) lands in $a0 here vs the target's $v0 throughout, forcing a
 * final `move v0,a0` before the epilogue that the target doesn't need.
 * `tools/regalloc.py` shows the real copy-chain (`i111 a0->v0`) but nothing
 * to break it: `result`'s own live range never crosses a call, so it isn't
 * dragged into a callee-saved register the way `human` (which DOES cross
 * the ControlHumanoid/character_balma_around_main_routine_ calls, hence
 * $s0) is — global-alloc simply prefers $a0 for this short-lived value over
 * $v0. Tried: declaration order (result before/after i/human), statement
 * order (result=Humans before/after i=0), an explicit early
 * `if (Humans <= 0) return Humans;` restructuring (WORSE: adds a spurious
 * `sll` + `move v0,zero` pair, growing the function by one instruction — the
 * do-while's own bottom `slt` naturally leaves 0 in $v0 at exit in the
 * ORIGINAL, but not when the guard is split into its own early return), and
 * a named `s16 zero = 0;` comparison local (GetArcData's lever) — none
 * moved the register. `tools/autorules.py` found no width-based win either.
 * One bounded permuter run (~300 iterations, --stop-on-zero -j4) plateaued
 * around score 300 (baseline 310) without reaching 0 — not re-run per the
 * attempt-cap guidance.
 */
extern s16 Humans;
extern Humanoid *HumanGroup[];
extern s16 VISIBLE_ENEMIES_;
extern void character_balma_around_main_routine_(void);
extern void ControlHumanoid(Humanoid *human);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ControlAllHumanoid", ControlAllHumanoid);
#else /* NON_MATCHING */

short ControlAllHumanoid(void)
{
    Humanoid *human;
    s16 i;
    s32 result;

    VISIBLE_ENEMIES_ = 0;
    i = 0;
    result = Humans;
    if (0 < result)
    do
    {
        human = HumanGroup[i];
        if ((*(u16 *)&human->attribute & 0x80) == 0)
        {
            if (human->type == 0x85)
            {
                character_balma_around_main_routine_();
                ControlHumanoid(human);
                character_balma_around_main_routine_();
            }
            else
            {
                ControlHumanoid(human);
            }
        }
        i = i + 1;
        result = 0;
    } while (i < Humans);
    return result;
}

#endif /* NON_MATCHING */
