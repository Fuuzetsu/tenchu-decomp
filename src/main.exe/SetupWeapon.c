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

    human->wepid[1] = -1;
    human->wepid[0] = -1;
    i = 0;
    do
    {
        human->weapon[i++] = 0;
    } while (i < 4);

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
        GetWeaponData(human, 0, human->wpatk, 1, -1);
    case 3:
        GetWeaponData(human, 0, human->wpatk, 0, -1);
        break;
    case 5:
    case 7:
        GetWeaponData(human, 0, human->wpatk + 1, -1, 0);
    case 4:
        GetWeaponData(human, 0xd, human->wpatk, 0, 2);
        GetWeaponData(human, 0xe, human->wpatk, 1, 3);
        break;
    case 10:
    case 0x20:
        GetWeaponData(human, 0xd, human->wpatk, 0, 0);
        GetWeaponData(human, 0xe, human->wpatk + 1, 1, 1);
        break;
    case 0x12:
        GetWeaponData(human, 0xe, 1, 1, -1);
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
        GetWeaponData(human, 0xd, human->wpatk, 0, 0);
        break;
    case 0x14:
        GetWeaponData(human, 0xd, 0x14, 0, 2);
        GetWeaponData(human, 1, 0x15, -1, 0);
        break;
    case WEP_TWIN_KATANA:
        GetWeaponData(human, 0xd, WEP_TWIN_KATANA, 0, 2);
        GetWeaponData(human, 0xe, 0x2b, -1, 1);
        GetWeaponData(human, 0xe, 0x2c, -1, 0);
        break;
    case 0x16:
    case 0x19:
    case 0x1c:
        GetWeaponData(human, 0, human->wpatk + 1, -1, 1);
        GetWeaponData(human, 0, human->wpatk + 2, -1, 0);
        human->weapon[0]->locate.coord.t[0] = human->width / 3;
        human->weapon[0]->locate.coord.t[1] = 0;
        human->weapon[0]->locate.coord.t[2] = 0;
        human->weapon[1]->locate.coord.t[0] = human->width / 3;
        human->weapon[1]->locate.coord.t[1] = 0;
        human->weapon[1]->locate.coord.t[2] = 0;
    case 0xc:
    case 0x13:
        GetWeaponData(human, 0xd, human->wpatk, 0, 2);
        break;
    case 0x1f:
        GetWeaponData(human, 0xd, 0x1f, 0, 2);
        GetWeaponData(human, 0xe, 0x1f, 1, 3);
        GetWeaponData(human, 0, 0x2d, -1, 0);
        break;
    case 0x30:
    case 0x31:
        GetWeaponData(human, 0xd, human->wpatk, -1, 0);
        break;
    case 0x32:
    case WEP_KATAOKA:
        GetWeaponData(human, 0xd, human->wpatk,
                      human->wpatk == WEP_KATAOKA ? 0 : -1, 0);
        GetWeaponData(human, 1, human->wpatk + 2, -1, 1);
        GetWeaponData(human, 0xe, human->wpatk + 1, -1, 2);
        break;
    default:
        return;
    }
}
