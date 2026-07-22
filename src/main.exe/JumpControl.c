#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void JumpControl(void);
 *     MOTION.C:367, 37 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct VECTOR *dtL;
 *     extern struct MotionManager *dtM;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct Humanoid *StagePlayer;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct SVECTOR *dtV;
 *     extern short dtPAD;
 * END PSX.SYM */

/*
 * JumpControl (0x8001d474, 0x248 bytes) — per-frame controller for the
 * jump/hang motion state (MOTION.C, called from ActCHASE/ActENGAGE/ActMOVE/
 * ActNORMAL). Kicks off a landing-dust effect at `*dtL` via FUN_80033bc0,
 * then checks whether the player's current motion (0x900) is still valid.
 * If not (GetMotionID returns a "not found" sentinel with the sign bit set),
 * the whole rest of the function is skipped. While already mid-jump-attack
 * (motID==0x607), lands into the roll-recovery motion (0x906) once the
 * active motion is more than 11 frames from its end, playing a footstep
 * sound (plus an extra landing sound if the player controls this Humanoid).
 * Otherwise (motID != 0x607): records the standing model's `id` into
 * ConflictObject[id].position's x/z (a "was standing here" landing-spot
 * cache) if it's registered, resets the jump target vector (`dtV->vy`),
 * switches to the base jump motion (0x900), then dispatches on the D-pad
 * bits to pick a directional jump motion (0x902..0x905, one of forward/
 * back/left/right) and pass a matching offset to MoveHumanoid — or, with no
 * direction held, zeroes `*dtV`'s x/z instead.
 *
 * Matching notes:
 *  - `GetMotionID` is declared here returning `u16` (its real prototype,
 *    GetMotionID.c, is `s16`) — this TU treats the raw result as an
 *    unsigned 16-bit pattern and tests its would-be sign bit manually via
 *    `(GetMotionID(...) << 16) < 0` (u16 promotes to `int`, so the shift
 *    moves bit 15 into bit 31 for a plain `sll`+`bltz`, no `sra` needed
 *    since the shifted value itself is never used afterward) — a per-TU
 *    divergent extern return type, same class as GetRealPad's caller in
 *    bow_shoot_logic (cookbook: "A caller-side extern's RETURN TYPE is an
 *    extension-position lever").
 *  - The `motID == 0x607` vs else split is a plain `if/else`: the small
 *    0x607 body is the fallthrough (physically first), the big else body
 *    is the branch target — standard cc1 if/else layout, no polarity
 *    inversion needed.
 *  - The D-pad dispatch is a plain `if/else if` ladder (0x1000, 0x4000,
 *    0x2000, 0x8000, else) — Ghidra's own nesting already has this right;
 *    m2c's flattened "if (bit) {...; return;}" four times is an equally
 *    valid but differently-shaped rendering of the SAME asm (every branch
 *    ends in a jump straight to the shared epilogue either way, since
 *    there is nothing after the dispatch). Each subsequent bitmask test is
 *    precomputed in the PRECEDING branch's delay slot (`andi` scheduled
 *    into the branch-not-taken slot) — a scheduler artifact, not a source
 *    shape; plain nested tests reproduce it without any hand-holding.
 *  - `Me_MOTION_C->model->object[0]->id` and `dtM->count` both check out
 *    against item.h's proven `ModelArchiveType *model`@0x58,
 *    `ModelType **object`@0x68, `s16 id`@0x58 (of ModelType), and
 *    MotionManager's proven `s16 count`@0x2.
 *  - `ConflictObjectType.position` (a VECTOR @0x4, so `.vx`@4/`.vz`@0xC of
 *    the slot) matches InsertConflict.c's/DeleteConflict.c's already-proven
 *    layout; redefined locally here per this repo's per-file convention.
 */
extern VECTOR *dtL;
extern MotionManager *dtM;
extern s16 motID;
extern SVECTOR *dtV;
extern u16 dtPAD;
extern s16 motMODE;
extern Humanoid *Me_MOTION_C;

extern void FUN_80033bc0(VECTOR *pos, int a, int b, int c);
extern u16 GetMotionID(MotionManager *mmp, s16 mid);
extern void Sound(Humanoid *h, int id);

void JumpControl(void)
{
    int id;

    FUN_80033bc0(dtL, 0x96, 0xC, 8);
    if ((GetMotionID(dtM, 0x900) << 16) < 0)
        return;

    if (motID == 0x607)
    {
        if (dtM->count < 0xB)
        {
            if (!((GetMotionID(dtM, 0x906) << 16) < 0))
            {
                motID = 0x906;
                motMODE = 0;
                MoveHumanoid(Me_MOTION_C, 0x7F, 0);
                if (Me_MOTION_C == StagePlayer)
                {
                    Sound(Me_MOTION_C, 0x48);
                }
                Sound(Me_MOTION_C, 0x17);
            }
        }
    }
    else
    {
        id = Me_MOTION_C->model->object[0]->id;
        if (id >= 0)
        {
            dtL->vx = ConflictObject[id].position.vx;
            dtL->vz = ConflictObject[id].position.vz;
        }
        motID = 0x900;
        motMODE = 0;
        dtV->vy = 0;
        if (dtPAD & 0x1000)
        {
            if (!((GetMotionID(dtM, 0x902) << 16) < 0))
            {
                motID = 0x902;
                motMODE = 0;
            }
            MoveHumanoid(Me_MOTION_C, 100, 0);
        }
        else if (dtPAD & 0x4000)
        {
            if (!((GetMotionID(dtM, 0x903) << 16) < 0))
            {
                motID = 0x903;
                motMODE = 0;
            }
            MoveHumanoid(Me_MOTION_C, -100, 0);
        }
        else if (dtPAD & 0x2000)
        {
            if (!((GetMotionID(dtM, 0x904) << 16) < 0))
            {
                motID = 0x904;
                motMODE = 0;
            }
            MoveHumanoid(Me_MOTION_C, 0, -100);
        }
        else if (dtPAD & 0x8000)
        {
            if (!((GetMotionID(dtM, 0x905) << 16) < 0))
            {
                motID = 0x905;
                motMODE = 0;
            }
            MoveHumanoid(Me_MOTION_C, 0, 100);
        }
        else
        {
            dtV->vz = 0;
            dtV->vx = 0;
        }
    }
}
