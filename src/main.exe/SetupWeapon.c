#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupWeapon(struct Humanoid *human);
 *     APPEAR.C:299, 61 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $s0       struct Humanoid * human
 *     reg   $a1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanDataType HumanData[63];
 * END PSX.SYM */

/*
 * SetupWeapon (0x8002a484) — initialise a humanoid's weapon ornaments.
 *
 * STATUS: NON_MATCHING — complete guarded pure-C reconstruction at 92.20
 * fuzzy similarity. The draft has the target's exact 1128-byte / 282-
 * instruction extent and reproduces every weapon-kind case and call. Only
 * 47 bytes remain, all in the leading four-slot clear and HumanData search,
 * where the target keeps the short index in a1 and schedules the clear into
 * the loop delay slot. Build with `NON_MATCHING=SetupWeapon ./Build`.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", SetupWeapon);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_a);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_12);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_9);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_14);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_2a);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_16);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_c);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_1f);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_30);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_32);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_6);

/* jump-table pool @ 0x80011818 (53 words; tables at 0x80011818) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 SetupWeapon_jtbl[53] = {
    0x8002A57C, 0x8002A57C, 0x8002A598, 0x8002A5D8,
    0x8002A5B4, 0x8002A8DC, 0x8002A5B4, 0x8002A8DC,
    0x8002A664, 0x8002A610, 0x8002A8DC, 0x8002A7E0,
    0x8002A8DC, 0x8002A8DC, 0x8002A8DC, 0x8002A664,
    0x8002A664, 0x8002A648, 0x8002A7E0, 0x8002A67C,
    0x8002A8DC, 0x8002A700, 0x8002A8DC, 0x8002A8DC,
    0x8002A700, 0x8002A8DC, 0x8002A8DC, 0x8002A700,
    0x8002A8DC, 0x8002A8DC, 0x8002A7FC, 0x8002A610,
    0x8002A8DC, 0x8002A664, 0x8002A664, 0x8002A664,
    0x8002A664, 0x8002A664, 0x8002A664, 0x8002A664,
    0x8002A664, 0x8002A6B0, 0x8002A8DC, 0x8002A8DC,
    0x8002A8DC, 0x8002A8DC, 0x8002A8DC, 0x8002A84C,
    0x8002A84C, 0x8002A864, 0x8002A8DC, 0x8002A8DC,
    0x8002A864,
};

#else /* NON_MATCHING */
typedef struct
{
    s16 type;
    u16 wepid;
    s16 turn;
    s16 life;
    s16 width;
    s16 height;
    void *mtbl;
    u8 *name;
    u32 *model;
} SetupWeaponHumanData;

typedef struct
{
    GsCOORDINATE2 locate;
} SetupWeaponOrnament;

extern SetupWeaponHumanData HumanData[63];
extern void GetWeaponData(Humanoid *human, s16 body, s16 wid, s32 wpid, int wep);

void SetupWeapon(Humanoid *human)
{
    s16 i;
    s16 weapon_kind;

    human->wepid[1] = -1;
    human->wepid[0] = -1;
    i = 0;
    do
    {
        human->weapon[i] = 0;
        i++;
    } while (i < 4);

    i = 0;
    while (HumanData[i].type != human->type)
    {
        i++;
    }
    weapon_kind = HumanData[i].wepid;
    human->weapon_kind = weapon_kind;

    switch (weapon_kind)
    {
    case 1:
    case 2:
        GetWeaponData(human, 0, *(s16 *)&human->weapon_kind, 1, -1);
    case 3:
        GetWeaponData(human, 0, *(s16 *)&human->weapon_kind, 0, -1);
        break;
    case 5:
    case 7:
        GetWeaponData(human, 0, human->weapon_kind + 1, -1, 0);
    case 4:
        GetWeaponData(human, 0xd, *(s16 *)&human->weapon_kind, 0, 2);
        GetWeaponData(human, 0xe, *(s16 *)&human->weapon_kind, 1, 3);
        break;
    case 10:
    case 0x20:
        GetWeaponData(human, 0xd, *(s16 *)&human->weapon_kind, 0, 0);
        GetWeaponData(human, 0xe, human->weapon_kind + 1, 1, 1);
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
    case 0x29:
        GetWeaponData(human, 0xd, *(s16 *)&human->weapon_kind, 0, 0);
        break;
    case 0x14:
        GetWeaponData(human, 0xd, 0x14, 0, 2);
        GetWeaponData(human, 1, 0x15, -1, 0);
        break;
    case 0x2a:
        GetWeaponData(human, 0xd, 0x2a, 0, 2);
        GetWeaponData(human, 0xe, 0x2b, -1, 1);
        GetWeaponData(human, 0xe, 0x2c, -1, 0);
        break;
    case 0x16:
    case 0x19:
    case 0x1c:
        GetWeaponData(human, 0, human->weapon_kind + 1, -1, 1);
        GetWeaponData(human, 0, human->weapon_kind + 2, -1, 0);
        ((SetupWeaponOrnament *)human->weapon[0])->locate.coord.t[0] = human->width / 3;
        ((SetupWeaponOrnament *)human->weapon[0])->locate.coord.t[1] = 0;
        ((SetupWeaponOrnament *)human->weapon[0])->locate.coord.t[2] = 0;
        ((SetupWeaponOrnament *)human->weapon[1])->locate.coord.t[0] = human->width / 3;
        ((SetupWeaponOrnament *)human->weapon[1])->locate.coord.t[1] = 0;
        ((SetupWeaponOrnament *)human->weapon[1])->locate.coord.t[2] = 0;
    case 0xc:
    case 0x13:
        GetWeaponData(human, 0xd, *(s16 *)&human->weapon_kind, 0, 2);
        break;
    case 0x1f:
        GetWeaponData(human, 0xd, 0x1f, 0, 2);
        GetWeaponData(human, 0xe, 0x1f, 1, 3);
        GetWeaponData(human, 0, 0x2d, -1, 0);
        break;
    case 0x30:
    case 0x31:
        GetWeaponData(human, 0xd, *(s16 *)&human->weapon_kind, -1, 0);
        break;
    case 0x32:
    case 0x35:
    {
        s32 initial_wpid;

        initial_wpid = 0;
        if (*(s16 *)&human->weapon_kind != 0x35)
        {
            initial_wpid = -1;
        }
        GetWeaponData(human, 0xd, *(s16 *)&human->weapon_kind, initial_wpid, 0);
        GetWeaponData(human, 1, human->weapon_kind + 2, -1, 1);
        GetWeaponData(human, 0xe, human->weapon_kind + 1, -1, 2);
        break;
    }
    default:
        return;
    }

}

#endif /* NON_MATCHING */
