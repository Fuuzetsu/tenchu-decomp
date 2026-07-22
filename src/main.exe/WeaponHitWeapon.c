#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void WeaponHitWeapon(struct ModelType *hand);
 *     MOTION.C:771, 25 src lines, frame 56 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     param $s3       struct ModelType * hand
 *     reg   $s2       struct VECTOR * p
 *     reg   $s4       short id
 *     reg   $s0       short i
 *     stack sp+16     struct SVECTOR pv
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct BattleType BattleDB[78];
 *     extern struct MotionManager *dtM;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

/*
 * WeaponHitWeapon (0x8001f324, 0x284 bytes) retries rejected weapon
 * contacts, knocks the accepted combatants back, emits ten blood particles,
 * advances the attack motion, and supplies clash feedback.
 *
 * The same-name demo body and its MOTION.C:771-795 line records expose the
 * original control shape: the successful handling path remains inside an
 * indefinite retry loop. Rejected slots continue; the accepted path breaks
 * after handling. That makes the blood-particle for-loop genuinely nested,
 * so loop.c naturally hoists ConflictObject's two address forms and the /100
 * magic constant before the retry loop. Direct ConflictObject indexing and
 * the source's natural `p = &ConflictObject[hand->id].position` then reproduce
 * all 161 retail instructions without carrier locals or allocation fences.
 */

extern BattleType BattleDB[];
extern MotionManager *dtM;
extern Humanoid *Me_MOTION_C;

extern short GetConflictResult(ModelType *model, short index);
extern void Sound(Humanoid *h, int id);
extern void PadShockAR(s32 a, s32 b, s32 c, s32 d);
extern void SetBleed(VECTOR *pos, SVECTOR *vel, int a, int col);

void WeaponHitWeapon(ModelType *hand)
{
    SVECTOR pv;
    short i;
    short id;
    VECTOR *p;

    if ((hand->attribute & 0x8000) != 0)
    {
        do
        {
            id = GetConflictResult(hand, -1);
            if (id < 0)
            {
                return;
            }
            if ((ConflictObject[id].size.pad & 1) == 0)
            {
                continue;
            }
            if ((Humanoid *)ConflictObject[id].common == Me_MOTION_C)
            {
                continue;
            }

            MoveHumanoid(Me_MOTION_C, -0x1E, 0);
            if ((Humanoid *)ConflictObject[id].common != (Humanoid *)1)
            {
                MoveHumanoid((Humanoid *)ConflictObject[id].common, -0x1E, 0);
            }

            p = &ConflictObject[hand->id].position;
            for (i = 0; i < 10; i++)
            {
                pv.vx = rand() % 100 - 50;
                pv.vy = rand() % 100 - 50;
                pv.vz = rand() % 100 - 50;
                SetBleed(p, &pv, rand() % 20 + 20, 0x7FFF);
            }

            hand->attribute = hand->attribute & 0xBFFF;
            dtM->loop = BattleDB[id].power / -3 - 1;
            Sound(Me_MOTION_C, 0x36);
            if (StagePlayer == Me_MOTION_C)
            {
                PadShockAR(0, 0xFF, 10, 0);
            }
            break;
        } while (1);
    }
}
