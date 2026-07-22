#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ControlAllHumanoid(void);
 *     HUMAN.C:97, 6 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

/*
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
 * `result` deliberately has two roles: it first holds Humans for the empty
 * loop return, then receives the loop-continuation comparison in
 * `while (result = i < Humans)`. The final false `slt` is therefore already
 * the function's zero return in $v0. Writing a separate `result = 0` in the
 * body gives the value an $a0 home and needs an extra `move v0,a0` in the
 * epilogue.
 */
extern s16 Humans;
extern s16 VISIBLE_ENEMIES_;
extern void character_balma_around_main_routine_(void);
extern void ControlHumanoid(Humanoid *human);

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
    } while (result = i < Humans);
    return result;
}
