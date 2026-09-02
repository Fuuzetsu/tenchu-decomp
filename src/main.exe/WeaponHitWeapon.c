#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void WeaponHitWeapon(struct ModelType *hand);
 *     MOTION.C:771, 25 src lines, frame 56 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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

extern Humanoid *Me_MOTION_C;

void WeaponHitWeapon(ModelType *hand)
{
    SVECTOR pv;
    short i;
    short id;
    VECTOR *p;

    if ((hand->attribute & MODEL_ATTR_CONFLICT) != 0)
    {
        do
        {
            id = GetConflictResult(hand, CONFLICT_NONE);
            if (id < 0)
            {
                return;
            }
            if ((ConflictObject[id].size.pad &
                 CONFLICT_HIT) == 0)
            {
                continue;
            }
            if (ConflictObject[id].common == Me_MOTION_C)
            {
                continue;
            }

            MoveHumanoid(Me_MOTION_C, -30, 0);
            if (ConflictObject[id].common != (void *)CONFLICT_OWNER_ITEM)
            {
                MoveHumanoid(ConflictObject[id].common, -30, 0);
            }

            p = &ConflictObject[hand->id].position;
            for (i = 0; i < 10; i++)
            {
                pv.vx = rand() % 100 - 50;
                pv.vy = rand() % 100 - 50;
                pv.vz = rand() % 100 - 50;
                SetBleed(p, &pv, rand() % 20 + 20, RGB24(0, 127, 255));
            }

            hand->attribute = hand->attribute & ~MODEL_ATTR_COLLIDE;
            /* Retail indexes BattleDB with the conflict-pool slot, not either
             * fighter's attack id. */
            dtM->loop = BattleDB[id].power / -3 - 1;
            Sound(Me_MOTION_C, SE_WEAPON_CLASH);
            if (StagePlayer == Me_MOTION_C)
            {
                PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX, RUMBLE_ATTACK_NORMAL, RUMBLE_RELEASE_NONE);
            }
            break;
        } while (1);
    }
}
