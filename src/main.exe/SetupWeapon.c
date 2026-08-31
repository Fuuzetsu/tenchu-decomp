#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupWeapon(struct Humanoid *human);
 *     APPEAR.C:299, 61 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct Humanoid * human
 *     reg   $a1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanDataType HumanData[63];
 * END PSX.SYM */

/*
 * SetupWeapon (0x8002a484) — initialise a humanoid's weapon ornaments.
 *
 * STATUS: MATCHING — exact 1128-byte / 282-instruction pure-C match.
 * Keeping the clear's increment in `weapon[i++]` gives old cc1 the narrow
 * working copy used by both the target's loop-delay-slot store and the
 * following rotated HumanData scan.
 */

void SetupWeapon(Humanoid *human)
{
    s16 i;
    s16 weapon_kind;

    human->wepid[WEAPON_HAND_1] = WEAPON_HAND_NONE;
    human->wepid[WEAPON_HAND_0] = WEAPON_HAND_NONE;
    i = 0;
    do
    {
        human->weapon[i++] = 0;
    } while (i < WEAPON_SLOT_COUNT);

    i = 0;
    while (HumanData[i].type != human->type)
    {
        i++;
    }
    weapon_kind = HumanData[i].wepid;
    human->wpatk = weapon_kind;

    switch (weapon_kind)
    {
    case 1:
    case 2:
        GetWeaponData(human, MODEL_PART_WAIST, human->wpatk,
                      WEAPON_HAND_1, WEAPON_SLOT_NONE);
    case 3:
        GetWeaponData(human, MODEL_PART_WAIST, human->wpatk,
                      WEAPON_HAND_0, WEAPON_SLOT_NONE);
        break;
    case 5:
    case 7:
        GetWeaponData(human, MODEL_PART_WAIST, human->wpatk + 1,
                      WEAPON_HAND_NONE, WEAPON_SLOT_ACTIVE_0);
    case 4:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, human->wpatk,
                      WEAPON_HAND_0, WEAPON_SLOT_INACTIVE_0);
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_1, human->wpatk,
                      WEAPON_HAND_1, WEAPON_SLOT_INACTIVE_1);
        break;
    case 10:
    case 0x20:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, human->wpatk,
                      WEAPON_HAND_0, WEAPON_SLOT_ACTIVE_0);
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_1, human->wpatk + 1,
                      WEAPON_HAND_1, WEAPON_SLOT_ACTIVE_1);
        break;
    case 0x12:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_1, 1,
                      WEAPON_HAND_1, WEAPON_SLOT_NONE);
    case 9:
    case 0x10:
    case 0x11:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case WEP_MEIOU:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, human->wpatk,
                      WEAPON_HAND_0, WEAPON_SLOT_ACTIVE_0);
        break;
    case 0x14:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, 0x14,
                      WEAPON_HAND_0, WEAPON_SLOT_INACTIVE_0);
        GetWeaponData(human, 1, 0x15,
                      WEAPON_HAND_NONE, WEAPON_SLOT_ACTIVE_0);
        break;
    case WEP_TWIN_KATANA:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, WEP_TWIN_KATANA,
                      WEAPON_HAND_0, WEAPON_SLOT_INACTIVE_0);
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_1, 0x2b,
                      WEAPON_HAND_NONE, WEAPON_SLOT_ACTIVE_1);
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_1, 0x2c,
                      WEAPON_HAND_NONE, WEAPON_SLOT_ACTIVE_0);
        break;
    case 0x16:
    case 0x19:
    case 0x1c:
        GetWeaponData(human, MODEL_PART_WAIST, human->wpatk + 1,
                      WEAPON_HAND_NONE, WEAPON_SLOT_ACTIVE_1);
        GetWeaponData(human, MODEL_PART_WAIST, human->wpatk + 2,
                      WEAPON_HAND_NONE, WEAPON_SLOT_ACTIVE_0);
        human->weapon[WEAPON_SLOT_ACTIVE_0]->locate.coord.t[0] = human->width / 3;
        human->weapon[WEAPON_SLOT_ACTIVE_0]->locate.coord.t[1] = 0;
        human->weapon[WEAPON_SLOT_ACTIVE_0]->locate.coord.t[2] = 0;
        human->weapon[WEAPON_SLOT_ACTIVE_1]->locate.coord.t[0] = human->width / 3;
        human->weapon[WEAPON_SLOT_ACTIVE_1]->locate.coord.t[1] = 0;
        human->weapon[WEAPON_SLOT_ACTIVE_1]->locate.coord.t[2] = 0;
    case 0xc:
    case 0x13:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, human->wpatk,
                      WEAPON_HAND_0, WEAPON_SLOT_INACTIVE_0);
        break;
    case 0x1f:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, 0x1f,
                      WEAPON_HAND_0, WEAPON_SLOT_INACTIVE_0);
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_1, 0x1f,
                      WEAPON_HAND_1, WEAPON_SLOT_INACTIVE_1);
        GetWeaponData(human, MODEL_PART_WAIST, 0x2d,
                      WEAPON_HAND_NONE, WEAPON_SLOT_ACTIVE_0);
        break;
    case 0x30:
    case 0x31:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, human->wpatk,
                      WEAPON_HAND_NONE, WEAPON_SLOT_ACTIVE_0);
        break;
    case 0x32:
    case WEP_KATAOKA:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, human->wpatk,
                      human->wpatk == WEP_KATAOKA ? WEAPON_HAND_0 : WEAPON_HAND_NONE,
                      WEAPON_SLOT_ACTIVE_0);
        GetWeaponData(human, 1, human->wpatk + 2,
                      WEAPON_HAND_NONE, WEAPON_SLOT_ACTIVE_1);
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_1, human->wpatk + 1,
                      WEAPON_HAND_NONE, WEAPON_SLOT_INACTIVE_0);
        break;
    default:
        return;
    }
}
