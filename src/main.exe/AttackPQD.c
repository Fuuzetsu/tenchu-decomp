#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackPQD(short sfrm, short efrm);
 *     MOTION.C:849, 23 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short sfrm
 *     param $a1       short efrm
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;

void AttackPQD(s16 sfrm, s16 efrm)
{
    Humanoid *human;
    s16 count;
    OrnamentType **weapons;
    OrnamentType *held;
    OrnamentType *stowed;
    s32 seid;

    human = Me_MOTION_C;
    count = dtM->count;
    weapons = human->weapon;
    if (count == efrm || efrm == MOTION_FRAME_ANY)
    {
        if (weapons[WEAPON_SLOT_INACTIVE_1] == 0)
            return;
        seid = CHAR_SE_WEAPON_CHANGE_B;
        held = (weapons[WEAPON_SLOT_INACTIVE_0] = human->weapon[WEAPON_SLOT_ACTIVE_0]);
        stowed = weapons[WEAPON_SLOT_INACTIVE_1];
        human->weapon[WEAPON_SLOT_ACTIVE_0] = stowed;
        weapons[WEAPON_SLOT_INACTIVE_1] = 0;
    }
    else
    {
        if (count != sfrm)
            return;
        if (weapons[WEAPON_SLOT_INACTIVE_0] == 0)
            return;
        seid = CHAR_SE_WEAPON_CHANGE_A;
        held = human->weapon[WEAPON_SLOT_ACTIVE_0];
        weapons[WEAPON_SLOT_INACTIVE_1] = held;
        human->weapon[WEAPON_SLOT_ACTIVE_0] = weapons[WEAPON_SLOT_INACTIVE_0];
        weapons[WEAPON_SLOT_INACTIVE_0] = 0;
    }
    Sound(human, seid);
}
