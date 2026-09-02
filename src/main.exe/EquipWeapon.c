#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void EquipWeapon(struct Humanoid *human, short mode);
 *     APPEAR.C:380, 33 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short mode
 * END PSX.SYM */

/* WEAPON_DRAWN raises ATTR_WEAPON_DRAWN; WEAPON_SHEATHED clears it. */
void EquipWeapon(Humanoid *human, short mode)
{
    OrnamentType **weapons;
    OrnamentType *active_weapon, *inactive_weapon_0, *inactive_weapon_1;
    OrnamentType *single_inactive_weapon;

    weapons = human->weapon;
    dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
    if (mode != WEAPON_SHEATHED)
    {
        if ((human->attribute & ATTR_WEAPON_DRAWN) != 0)
        {
            return;
        }
        human->attribute = human->attribute | ATTR_WEAPON_DRAWN;
    }
    else
    {
        if ((human->attribute & ATTR_WEAPON_DRAWN) == 0)
        {
            return;
        }
        human->attribute = human->attribute & ~ATTR_WEAPON_DRAWN;
    }
    switch (human->wpatk)
    {
    case KODATI:
    case JYUTE:
    case EN:
    case KATANA_2:
        active_weapon = weapons[WEAPON_SLOT_ACTIVE_0];
        inactive_weapon_0 = weapons[WEAPON_SLOT_INACTIVE_0];
        inactive_weapon_1 = weapons[WEAPON_SLOT_INACTIVE_1];
        weapons[WEAPON_SLOT_INACTIVE_0] = active_weapon;
        active_weapon = weapons[WEAPON_SLOT_ACTIVE_1];
        weapons[WEAPON_SLOT_ACTIVE_0] = inactive_weapon_0;
        weapons[WEAPON_SLOT_ACTIVE_1] = inactive_weapon_1;
        weapons[WEAPON_SLOT_INACTIVE_1] = active_weapon;
        break;
    case KOZUKA:
    case NINJA:
    case KEITOU:
    case KATANA_0:
    case HOUTOU:
    case KATANA_1:
        single_inactive_weapon = weapons[WEAPON_SLOT_INACTIVE_0];
        active_weapon = weapons[WEAPON_SLOT_ACTIVE_0];
        weapons[WEAPON_SLOT_ACTIVE_0] = single_inactive_weapon;
        weapons[WEAPON_SLOT_INACTIVE_0] = active_weapon;
        break;
    }
}
