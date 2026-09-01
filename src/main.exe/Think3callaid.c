#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "humanoid.h"
#include "game_globals.h"
#include "appear.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3callaid(void);
 *     THINK_3.C:31, frame 40 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       struct Humanoid * human
 *     reg   $v1       struct Humanoid * human
 *     reg   $a0       short type
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern int StageID;
 *     extern short Degree;
 *     extern short (*Think1Func[10])();
 *     extern short (*Think2Func[5])();
 *     extern short (*Think3Func[10])();
 *     extern struct PADtype *Pad;
 *     extern short (*Think4Func[6])();
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
 * (Distance < 16500) just forwards to Think3escape(); otherwise "calls for
 * aid": spawns a fresh Humanoid via BreedLife (a random ally/foe type from
 * AIDHumanType[], indexed by StageID and a coin-flip), kills the CURRENT
 * character (Me_THINK_C), and possesses the new Humanoid — copying its
 * think[]/attribute/pad, equipping a weapon, kicking off a motion, and (a
 * RETAIL-ONLY addition absent from the demo, so PSX.SYM doesn't mention it)
 * bumping StageEnemies/StageCitizens by +1/-1 when the new character is a
 * civilian-turned-enemy (character_kind & 0xF0 == PAGE_CIVILIAN).
 *
 * `newhuman->think[0..3]` takes index 4 from all four tables — ThinkDB
 * names that row 1B-WATCH / 2B-CONTACT / 3B-ATK-CHASE / 4B-CONTACT, so the
 * summoned fighter watches, closes, chases its attacker and holds contact
 * (see item.h) — PSX.SYM's original `short (*think[4])()` field, shifted
 * from demo +0x58 to retail +0x60 by the expanded MapVector.
 *
 * The possession assignment intentionally updates Humanoid.attribute in the
 * same expression that installs Me_THINK_C, preserving the target load/store
 * schedule.
 *
 * `AIDHumanType[StageID * 2 + r % 2]` is a signed `s16` table (`lh`,
 * unlike the item-TU's usual unsigned tables) — passed to BreedLife's
 * `short type` parameter directly, no narrowing cast needed since the array
 * element is already the right width.
 *
 * The `rand() % 2` "coin flip" is a four-instruction srl+addu+sra+sll signed
 * remainder-by-2 idiom (rounding toward zero) — automatic codegen for a
 * plain `%` by a compile-time constant power of two, not hand-written.
 *
 * Matching notes (fixes applied, all confirmed via matchdiff/asmdiff):
 *  - The whole function's return value must NOT funnel through one shared
 *    `ret` variable returned once at the end (Ghidra's literal rendering):
 *    that made cc1 converge BOTH branches through a single register (here
 *    $a0, needing a final generic sign-extend-and-widen at a shared join),
 *    while the target widens the Think3escape() result INLINE before
 *    jumping to a plain epilogue, and the else-branch's `ret` (always a
 *    small constant) needs no widening at all. Splitting into
 *    `return Think3escape();` (early, inside the if) and a second,
 *    branch-LOCAL `s16 ret;` returned at the end of the else block fixed
 *    this (the InsertConflict/DrawBG "two early returns" cookbook rule).
 *  - `s16 *aid = AIDHumanType;` declared and assigned BEFORE `rand()` (not
 *    `AIDHumanType[...]` indexed inline after the call) is required for the
 *    table base address to be computed EARLY and survive in a
 *    callee-saved register across the call, matching target's
 *    `lui/addiu` into $s0 interleaved with `SR = SR_UNSEEN;` before the `jal rand`
 *    (the "table lookup gets its own named local pointer" cookbook rule).
 *  - Keep a second `s16 *type_ptr` for the selected table entry, then load the
 *    PSX.SYM-proven `s16 type` in a separate statement. Folding those two
 *    identities into one dereference lets local allocation reuse $a0 for both
 *    the address and value; the explicit pointer restores retail's address in
 *    $v0, `type` in $a0, StageID offset in $v1, and Me_THINK_C base in $a2.
 *  - Keep possession and the attribute flag as one assignment-expression
 *    lvalue: `(Me_THINK_C = newhuman)->... |= ATTR_CUSTOMAI`. Splitting
 *    this through Ghidra's `uVar1` or into an independent global assignment
 *    loses the dependency that places retail's Me_THINK_C store between the
 *    halfword load and update, and exchanges the Think3/Think4 callback and
 *    attribute registers. The store is the u16 view of Humanoid's
 *    signed attribute, so the explicit `u16` temporary is unnecessary.
 *  - An empty one-shot loop between the Think1Func and Think2Func stores was a
 *    useful intermediate diagnostic: it restored the missing load-delay slot
 *    while the possession/attribute dependency was still split. Once that
 *    assignment-expression lvalue was recovered, removing the artificial loop
 *    placed `move $a0,$s0` and the sole `nop` exactly. Always re-test and remove
 *    a scheduler fence after fixing the producer identities it was masking.
 */
extern Humanoid *Me_THINK_C;
/* Per-stage reinforcement pair (StageID*2 + coin flip) — the stage's
 * own guard faction (retail data): rouban/rounin, ninja A+B, rouban,
 * Manji cultists, pirates, tengu, oni, kabane, kerai, asigaru, sisi. */
extern s16 AIDHumanType[]; /* [][2] in think_alarm_reaction_.c; flat here for the required pointer idiom above */
extern int rand(void);
extern s16 Think3escape(void);

short Think3callaid(void)
{
    Humanoid *human;
    Humanoid *newhuman;
    s32 r;

    if (Distance < 16500)
    {
        if (SR != SR_GONE)
        {
            SR = SR_NONE;
        }
        return Think3escape();
    }
    else
    {
        s16 ret;
        s16 *aid = AIDHumanType;
        s16 *type_ptr;
        s16 type;
        ThinkFunc func;

        SR = SR_UNSEEN;
        r = rand();
        /* Byte-offset spelling: byte-required (plain indexing recolors the
         * base register; measured). */
        type_ptr = (s16 *)((u8 *)aid + ((r % 2) * 2 + StageID * 4));
        type = *type_ptr;
        newhuman = BreedLife(type,
                             Me_THINK_C->locate->vx,
                             Me_THINK_C->locate->vy,
                             Me_THINK_C->locate->vz,
                             (s32)Me_THINK_C->rotate->vy + (s32)Degree);
        human = Me_THINK_C;
        newhuman->target = human->target;
        KillHumanoid(human);
        newhuman->think[0] = Think1Func[THINK1_WATCH];
        newhuman->think[1] = Think2Func[THINK2_CONTACT];
        newhuman->think[2] = Think3Func[THINK3_ATK_CHASE];
        Pad = &newhuman->pad;
        func = Think4Func[THINK4_CONTACT];
        (Me_THINK_C = newhuman)->attribute |= ATTR_CUSTOMAI;
        newhuman->think[3] = func;
        EquipWeapon(newhuman, WEAPON_DRAWN);
        SetNowMotion(Me_THINK_C, MOT_ENGAGE_STANCE, 1);
        Attrib = Me_THINK_C->attribute | PHASE_ALERT;
        ret = 0;
        if ((Me_THINK_C->type & PAGE_MASK) == PAGE_CIVILIAN)
        {
            StageEnemies++;
            StageCitizens--;
            ret = 0;
        }
        return ret;
    }
}
