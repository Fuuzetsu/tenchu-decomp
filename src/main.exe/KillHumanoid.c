#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void KillHumanoid(struct Humanoid *human);
 *     HUMAN.C:78, 15 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

extern void DisposeModelArchive(ModelArchiveType *mad);
extern void DisposeMotionManager(MotionManager *mm);
extern void vfree(void *p);

void KillHumanoid(Humanoid *human)
{
    short i;

    if (human != 0)
    {
        DeleteConflict(human->model->object[MODEL_PART_WAIST]);
        DisposeModelArchive(human->model);
        DisposeMotionManager(human->motion);
        DisposeWeapon(human);
        dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
        vfree(human);
        for (i = 0; i < Humans; i++)
        {
            if (HumanGroup[i] == human)
            {
                break;
            }
        }
        if (i < Humans)
        {
            Humans--;
            HumanGroup[i] = HumanGroup[Humans];
        }
    }
}
