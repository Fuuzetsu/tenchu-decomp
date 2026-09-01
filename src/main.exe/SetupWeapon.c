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

    human->wepid[WEAPON_HAND_1] = WEAPON_HAND_NONE;
    human->wepid[WEAPON_HAND_0] = WEAPON_HAND_NONE;
    i = 0;
    do
    {
        human->weapon[i++] = 0;
    } while (i < N_WEAPON_SLOTS);

    i = 0;
    while (HumanData[i].type != human->type)
    {
        i++;
    }
    human->wpatk = HumanData[i].wepid;

    switch (human->wpatk)
    {
    case CLAW:
    case FIST:
        GetWeaponData(human, MODEL_PART_WAIST, human->wpatk,
                      WEAPON_HAND_1, WEAPON_SLOT_NONE);
    case JAW:
        GetWeaponData(human, MODEL_PART_WAIST, human->wpatk,
                      WEAPON_HAND_0, WEAPON_SLOT_NONE);
        break;
    case JYUTE:
    case EN:
        GetWeaponData(human, MODEL_PART_WAIST, human->wpatk + 1,
                      WEAPON_HAND_NONE, WEAPON_SLOT_ACTIVE_0);
    case KODATI:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, human->wpatk,
                      WEAPON_HAND_0, WEAPON_SLOT_INACTIVE_0);
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_1, human->wpatk,
                      WEAPON_HAND_1, WEAPON_SLOT_INACTIVE_1);
        break;
    case JYURUR:
    case CROWR:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, human->wpatk,
                      WEAPON_HAND_0, WEAPON_SLOT_ACTIVE_0);
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_1, human->wpatk + 1,
                      WEAPON_HAND_1, WEAPON_SLOT_ACTIVE_1);
        break;
    case SABRE:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_1, CLAW,
                      WEAPON_HAND_1, WEAPON_SLOT_NONE);
    case ANDON:
    case IKARI:
    case BOU:
    case YARI:
    case KABUTUTI:
    case SASUMATA:
    case HALBERT:
    case KON:
    case NAGI:
    case ENGETU:
    case SEVEN:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, human->wpatk,
                      WEAPON_HAND_0, WEAPON_SLOT_ACTIVE_0);
        break;
    case KEITOU:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, KEITOU,
                      WEAPON_HAND_0, WEAPON_SLOT_INACTIVE_0);
        GetWeaponData(human, 1, KEITOUB,
                      WEAPON_HAND_NONE, WEAPON_SLOT_ACTIVE_0);
        break;
    case KATANAL:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, KATANAL,
                      WEAPON_HAND_0, WEAPON_SLOT_INACTIVE_0);
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_1, SAYAL,
                      WEAPON_HAND_NONE, WEAPON_SLOT_ACTIVE_1);
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_1, TUKAL,
                      WEAPON_HAND_NONE, WEAPON_SLOT_ACTIVE_0);
        break;
    case KATANA_0:
    case HOUTOU:
    case KATANA_1:
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
    case KOZUKA:
    case NINJA:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, human->wpatk,
                      WEAPON_HAND_0, WEAPON_SLOT_INACTIVE_0);
        break;
    case KATANA_2:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, KATANA_2,
                      WEAPON_HAND_0, WEAPON_SLOT_INACTIVE_0);
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_1, KATANA_2,
                      WEAPON_HAND_1, WEAPON_SLOT_INACTIVE_1);
        GetWeaponData(human, MODEL_PART_WAIST, TUKAANI,
                      WEAPON_HAND_NONE, WEAPON_SLOT_ACTIVE_0);
        break;
    case TEPPO:
    case GUN:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, human->wpatk,
                      WEAPON_HAND_NONE, WEAPON_SLOT_ACTIVE_0);
        break;
    case YUMI:
    case KATAYUMI:
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_0, human->wpatk,
                      human->wpatk == KATAYUMI ? WEAPON_HAND_0 : WEAPON_HAND_NONE,
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
