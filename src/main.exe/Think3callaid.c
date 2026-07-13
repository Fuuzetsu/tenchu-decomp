#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3callaid(void);
 *     THINK_3.C:31, frame 40 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $s0       struct Humanoid * human
 *     reg   $v1       struct Humanoid * human
 *     reg   $a0       short type
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern int StageID;
 *     extern short Degree;
 *     extern struct PADtype *Pad;
 *     extern short Attrib;
 *     extern short StageEnemies;
 *     extern short StageCitizens;
 * END PSX.SYM */

/*
 * STATUS: MATCHING — pure C, 408/408 bytes exact (102 instructions).
 *
 * Think3callaid (0x8002cddc, 0x198 bytes) — same "think" TU as
 * Think3chase.c/Think3escape.c/Think1trace.c (s16 return; gp-relative
 * Me_THINK_C/Distance/SR/Degree/Pad/Attrib — see gpsyms). When close
 * (Distance < 0x4074) just forwards to Think3escape(); otherwise "calls for
 * aid": spawns a fresh Humanoid via BreedLife (a random ally/foe type from
 * AIDHumanType[], indexed by StageID and a coin-flip), kills the CURRENT
 * character (Me_THINK_C), and possesses the new Humanoid — copying its
 * think[]/attribute/pad, equipping a weapon, kicking off a motion, and (a
 * RETAIL-ONLY addition absent from the demo, so PSX.SYM doesn't mention it)
 * bumping StageEnemies/StageCitizens by +1/-1 when the new character is a
 * civilian-turned-enemy (character_kind & 0xF0 == 0x90).
 *
 * `human = (Humanoid *)Me_THINK_C;` bridges this TU's own `character_state *`
 * view of the current character to the item-TU `Humanoid *` that
 * KillHumanoid/BreedLife/EquipWeapon/SetNowMotion all take — both describe
 * the same runtime object from different TUs (established convention; see
 * the cookbook's gp/cross-TU-struct notes).
 *
 * `human_00->think[0..3] = Think1Func[4]/Think2Func[4]/Think3Func[4]/
 * Think4Func[4]` — item.h's Humanoid.think[4] (new field, this function's
 * own proof) at 0x60, matching game_types.h's character_state twin
 * (think_setting0..3) and Ghidra's own independently-built Humanoid
 * (`pointer *think[4]` at the identical offset).
 *
 * The possession assignment intentionally updates the attribute through
 * character_state's unsigned twin (`some_character_marker_thing`), which
 * emits the target `lhu` against item.h's signed `s16 attribute` at the same
 * offset (the same cross-TU divergence as ControlAllHumanoid/HumanActionControl).
 *
 * `AIDHumanType[StageID * 2 + iVar4 % 2]` is a signed `s16` table (`lh`,
 * unlike the item-TU's usual unsigned tables) — passed to BreedLife's
 * `short type` parameter directly, no narrowing cast needed since the array
 * element is already the right width.
 *
 * The `rand() % 2` "coin flip" is a two-instruction srl+addu+sra+sll signed
 * remainder-by-2 idiom (rounding toward zero) — automatic codegen for a
 * plain `%` by a compile-time constant power of two, not hand-written.
 *
 * Matching notes (fixes applied, all confirmed via matchdiff/asmdiff):
 *  - The whole function's return value must NOT funnel through one shared
 *    `sVar3` variable returned once at the end (Ghidra's literal rendering):
 *    that made cc1 converge BOTH branches through a single register (here
 *    $a0, needing a final generic sign-extend-and-widen at a shared join),
 *    while the target widens the Think3escape() result INLINE before
 *    jumping to a plain epilogue, and the else-branch's `sVar3` (always a
 *    small constant) needs no widening at all. Splitting into
 *    `return Think3escape();` (early, inside the if) and a second,
 *    branch-LOCAL `s16 sVar3;` returned at the end of the else block fixed
 *    this (the InsertConflict/DrawBG "two early returns" cookbook rule).
 *  - `s16 *aid = AIDHumanType;` declared and assigned BEFORE `rand()` (not
 *    `AIDHumanType[...]` indexed inline after the call) is required for the
 *    table base address to be computed EARLY and survive in a
 *    callee-saved register across the call, matching target's
 *    `lui/addiu` into $s0 interleaved with `SR = -1;` before the `jal rand`
 *    (the "table lookup gets its own named local pointer" cookbook rule).
 *  - Keep a second `s16 *type_ptr` for the selected table entry, then load the
 *    PSX.SYM-proven `s16 type` in a separate statement. Folding those two
 *    identities into one dereference lets local allocation reuse $a0 for both
 *    the address and value; the explicit pointer restores retail's address in
 *    $v0, `type` in $a0, StageID offset in $v1, and Me_THINK_C base in $a2.
 *  - Keep possession and the attribute flag as one assignment-expression
 *    lvalue: `(Me_THINK_C = (character_state *)human_00)->... |= 4`. Splitting
 *    this through Ghidra's `uVar1` or into an independent global assignment
 *    loses the dependency that places retail's Me_THINK_C store between the
 *    halfword load and update, and exchanges the Think3/Think4 callback and
 *    attribute registers. The field is the character_state twin of Humanoid's
 *    signed attribute, so the explicit `u16` temporary is unnecessary.
 *  - An empty one-shot loop between the Think1Func and Think2Func stores was a
 *    useful intermediate diagnostic: it restored the missing load-delay slot
 *    while the possession/attribute dependency was still split. Once that
 *    assignment-expression lvalue was recovered, removing the artificial loop
 *    placed `move $a0,$s0` and the sole `nop` exactly. Always re-test and remove
 *    a scheduler fence after fixing the producer identities it was masking.
 */
extern character_state *Me_THINK_C;
extern s32 Distance;
extern s16 SR;
extern s32 StageID;
extern s16 Degree;
extern PADtype *Pad;
extern u16 Attrib;
extern u16 StageEnemies;
extern u16 StageCitizens;
extern s16 AIDHumanType[];
extern think_func_ *Think1Func[];
extern think_func_ *Think2Func[];
extern think_func_ *Think3Func[];
extern think_func_ *Think4Func[];
extern int rand(void);
extern s16 Think3escape(void);
extern Humanoid *BreedLife(s16 type, s32 x, s32 y, s32 z, s32 r);
extern void KillHumanoid(Humanoid *human);
extern void EquipWeapon(Humanoid *human, s16 mode);
extern s16 SetNowMotion(Humanoid *human, s16 mid, s16 move);

short Think3callaid(void)
{
    Humanoid *human;
    Humanoid *human_00;
    s32 iVar4;

    if (Distance < 0x4074)
    {
        if (SR != -2)
        {
            SR = 0;
        }
        return Think3escape();
    }
    else
    {
        s16 sVar3;
        s16 *aid = AIDHumanType;
        s16 *type_ptr;
        s16 type;
        think_func_ *ppuVar2;

        SR = -1;
        iVar4 = rand();
        type_ptr = (s16 *)((u8 *)aid + ((iVar4 % 2) * 2 + StageID * 4));
        type = *type_ptr;
        human_00 = BreedLife(type,
                             Me_THINK_C->some_kind_of_current_position->vx,
                             Me_THINK_C->some_kind_of_current_position->vy,
                             Me_THINK_C->some_kind_of_current_position->vz,
                             (s32)Me_THINK_C->something_about_player_rotation_perhaps->character_rotation + (s32)Degree);
        human = (Humanoid *)Me_THINK_C;
        human_00->target = human->target;
        KillHumanoid(human);
        human_00->think[0] = Think1Func[4];
        human_00->think[1] = Think2Func[4];
        human_00->think[2] = Think3Func[4];
        Pad = &human_00->pad;
        ppuVar2 = Think4Func[4];
        (Me_THINK_C = (character_state *)human_00)->some_character_marker_thing |= 4;
        human_00->think[3] = ppuVar2;
        EquipWeapon(human_00, 1);
        SetNowMotion((Humanoid *)Me_THINK_C, 0x501, 1);
        Attrib = Me_THINK_C->some_character_marker_thing | 2;
        sVar3 = 0;
        if ((Me_THINK_C->character_kind & 0xF0) == 0x90)
        {
            StageEnemies = StageEnemies + 1;
            StageCitizens = StageCitizens - 1;
            sVar3 = 0;
        }
        return sVar3;
    }
}
