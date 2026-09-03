#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"
#include "sound.h"

/*
 * The demo symbols place GetAttackDBID before GetWeaponData. The shipped
 * executable moves it after SetupWeapon; the definitions below follow the
 * retail order, with both orders recorded in the translation-unit manifest.
 */

extern s16 ARMOUR_EQUIPPED_;
extern character_kind smode;
extern s16 sstage;
extern u8 str_rikimaua[];                     /* RIKIMAUA */
extern u8 str_ayamea[];                       /* AYAMEA */
extern u8 str_ayames[];                       /* AYAMES */
extern char fmt_motion_stage_amd[];           /* %sMOTION\\STAGE%d.AMD */
extern char path_human[];                     /* K:\\WORK\\CDIMAGE\\HUMAN\\ */
extern char path_human_motion_common_amd[];   /* K:\\WORK\\CDIMAGE\\HUMAN\\MOTION\\COMMON.AMD */
extern char path_human_motion_rikimaru_amd[]; /* K:\\WORK\\CDIMAGE\\HUMAN\\MOTION\\RIKIMARU.AMD */
extern char path_human_motion_ayame_amd[];    /* K:\\WORK\\CDIMAGE\\HUMAN\\MOTION\\AYAME.AMD */
extern char msg_illigal_character_type[];     /* ILLIGAL CHARACTER TYPE */
extern char fmt_mad[];                        /* %s%s.MAD */
extern char fmt_tmd[];                        /* %s%s.TMD */
extern char path_human_weapon[];              /* K:\\WORK\\CDIMAGE\\HUMAN\\WEAPON\\ */

extern int strcmp(const char *a, const char *b);
extern int sprintf(char *dst, const char *fmt, ...);
extern OrnamentType *LoadOrnament(u_long *adr);
extern void DisposeOrnament(OrnamentType *objp);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupAppearance(short mode, short stage);
 *     APPEAR.C:109, 60 src lines, frame 160 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s7       short mode
 *     param $fp       short stage
 *     reg   $s2       short i
 *     reg   $s1       short j
 *     stack sp+16     unsigned char [100] name
 *     reg   $a0       unsigned char * pt
 *
 * Globals it touches, as the original declared them:
 *     extern short NowStage;
 *     extern unsigned char gNannido;
 *     extern short EngageLevel;
 *     extern struct HumanDataType HumanData[63];
 *     extern struct WeaponModelType WeaponModel[41];
 *     extern struct MotionPackType *CommonMotion;
 *     extern struct MotionPackType *PlayerMotion;
 *     extern struct MotionPackType *StageMotion;
 * END PSX.SYM */

void SetupAppearance(character_kind character, short stage)
{
    short i;
    short j;
    u8 name[100];
    u8 *pt;
    u8 armour;

    NowStage = stage;
    pt = (u8 *)TENCHU_PERSISTENT_STATE_ADDRESS;
    EngageLevel = 3 - ((TLinkInfo *)pt)->Nannido;
    armour = ((TLinkInfo *)pt)->selItem[ITEM_ARMOUR];
    if (armour != 0)
    {
        HumanData[0].name = str_rikimaua;
        HumanData[1].name = armour != ITEM_INFINITE ? str_ayamea : str_ayames;
        /* Wearing the armour consumes it from the mission loadout. The
         * TLinkInfo view stays absolute because `pt` is repurposed below. */
        ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->selItem[ITEM_ARMOUR] = 0;
        ARMOUR_EQUIPPED_ = -1;
    }

    i = 0;
    while (HumanData[i].type != CHARACTER_KIND_END)
    {
        if (HumanData[i].model != 0)
        {
            vfree(HumanData[i].model);
            HumanData[i].model = 0;
            j = 0;
            while (HumanData[j].type != CHARACTER_KIND_END)
            {
                if (strcmp((char *)HumanData[i].name,
                           (char *)HumanData[j].name) == 0)
                {
                    HumanData[j].model = 0;
                }
                j++;
            }
        }
        HumanData[i].model = 0;
        HumanData[i].mtbl->motion = 0;
        i++;
    }

    i = 0;
    while (WeaponModel[i].wid != WEAPON_KIND_END)
    {
        if (WeaponModel[i].model != 0)
        {
            vfree(WeaponModel[i].model);
        }
        WeaponModel[i].model = 0;
        i++;
    }

    if (stage < 0)
    {
        if (CommonMotion != 0)
        {
            vfree(CommonMotion);
            CommonMotion = 0;
        }
        if (PlayerMotion != 0)
        {
            vfree(PlayerMotion);
            PlayerMotion = 0;
        }
        if (StageMotion != 0)
        {
            vfree(StageMotion);
            StageMotion = 0;
        }
    }
    else
    {
        if (StageMotion != 0)
        {
            vfree(StageMotion);
        }
        sstage = stage;
        sprintf((char *)name, fmt_motion_stage_amd, path_human, (int)stage);
        StageMotion = LoadMotion(FileRead(name));
        if (stage != 0)
        {
            if (CommonMotion == 0)
            {
                CommonMotion = LoadMotion(FileRead((u8 *)path_human_motion_common_amd));
                SetupMotionRegist(MOTcommon);
            }
            if (PlayerMotion != 0)
            {
                if (character != smode)
                {
                    vfree(PlayerMotion);
                    PlayerMotion = 0;
                }
                if (PlayerMotion != 0)
                {
                    return;
                }
            }
            smode = character;
            if (character == RIKIMARU_0)
            {
                pt = (u8 *)path_human_motion_rikimaru_amd;
            }
            else
            {
                pt = (u8 *)path_human_motion_ayame_amd;
            }
            PlayerMotion = LoadMotion(FileRead(pt));
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * SetupCharacterParameter(short type, struct Humanoid *human);
 *     APPEAR.C:173, 25 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       short type
 *     param $s1       struct Humanoid * human
 *     reg   $a0       int idx
 *     reg   $a2       short * idtbl
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanDataType HumanData[63];
 *     extern short NowStage;
 *     extern short *StageAppearance[10];
 * END PSX.SYM */

Humanoid *SetupCharacterParameter(character_kind type, Humanoid *human)
{
    int idx;
    character_kind *idtbl;

    idx = 0;
    while (HumanData[idx].type != CHARACTER_KIND_END)
    {
        if (HumanData[idx].type == type)
        {
            break;
        }
        idx++;
    }
    human->turn = HumanData[idx].turn;
    human->width = HumanData[idx].width;
    human->height = HumanData[idx].height;
    if (HumanData[idx].mtbl->motion == 0)
    {
        SetupMotionRegist(HumanData[idx].mtbl);
    }
    human->motion = SetupMotionManager(human->model, HumanData[idx].mtbl);
    human->life = human->lifemax = HumanData[idx].life;

    idx = -1;
    /* (u16): the sltiu range test is in the bytes. */
    if ((u16)type >= N_PLAYABLE_CHARACTERS)
    {
        idtbl = StageAppearance[NowStage];
        idx = 0;
        while (idtbl[idx] != type)
        {
            if (idtbl[idx] == CHARACTER_KIND_END)
            {
                break;
            }
            idx++;
        }
    }
    /* VAB program 5 for player/partner (idx -1), then 6+ per stage. */
    human->sound = SOUND_PROGRAM_BASE(idx + 6);
    return human;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * BreedLife(short type, long x, long y, long z, long r);
 *     APPEAR.C:202, 50 src lines, frame 160 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s4       short type
 *     param $s5       long x
 *     param $s7       long y
 *     param $s6       long z
 *     param stack+16  long r
 *     reg   $v0       long r
 *     reg   $s0       struct Humanoid * human
 *     reg   $s2       unsigned long * model
 *     reg   $s0       unsigned long idx
 *     stack sp+16     unsigned char [100] name
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanDataType HumanData[63];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

Humanoid *BreedLife(character_kind type, long x, long y, long z, long r)
{
    /* PSX.SYM and the retail multiply both show a full-width counter. */
    u32 idx;
    HumanDataType *row;
    HumanDataType *base;
    u_long *model;
    HumanDataType *pp;
    HumanDataType *tbl;
    HumanDataType *q;
    Humanoid *human;
    u8 name[100];

    idx = 0;
    if (HumanData[0].type == CHARACTER_KIND_END)
        goto illegal_type;
    while (HumanData[idx].type != CHARACTER_KIND_END)
    {
        base = HumanData;
        if (base[idx].type == type)
            break;
        idx++;
    }
    if (base[idx].type != CHARACTER_KIND_END)
        goto type_found;
illegal_type:
    SystemOut(msg_illigal_character_type);
type_found:
    tbl = HumanData;
    pp = &tbl[idx];
    model = pp->model;
    if (model == 0)
    {
        sprintf((char *)name, fmt_mad, path_human, pp->name);
        model = FileRead(name);
        pp->model = model;
        if (HumanData[0].type != CHARACTER_KIND_END)
        {
            q = pp;
            row = HumanData;
        scan_next:
            if (strcmp((char *)q->name, (char *)row->name) == 0)
            {
                row->model = model;
            }
            row++;
            if (row->type != CHARACTER_KIND_END)
                goto scan_next;
        }
    }

    human = CreateHumanoid(type, model);
    human->point[HUMANOID_HOME_X] = x;
    human->model->locate.coord.t[0] = x;
    human->model->locate.coord.t[1] = GetAreaMapLevel(
        GlobalAreaMap, x, y, z, AREA_LEVEL_STEP_DOWN);
    human->point[HUMANOID_HOME_Z] = z;
    human->model->locate.coord.t[2] = z;
    human->model->rotate.vy = r;
    UpdateCoordinate((ModelType *)human->model);

    if (type == NINJA_0)
    {
        human->item[ITEM_KUSURI] = 1;
    }
    if (type >= ANI)
    {
        if (type >= ARROW)
            return human;
        if (type < S1)
            goto done;
        goto high_type;
    }

    if (type < HANBE)
    {
        if (type >= RIKIMARU_1)
            return human;
        if (type < 0)
            return human;
    }
    human->attribute = human->attribute | PHASE_ALERT;
    EquipWeapon(human, WEAPON_DRAWN);
    SetNowMotion(human, MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
    return human;
high_type:
    human->attribute = human->attribute | ATTR_FLOAT;
done:
    return human;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetWeaponData(struct Humanoid *human, short body, short wid, short wpid, int wep);
 *     APPEAR.C:270, 25 src lines, frame 144 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct Humanoid * human
 *     param $s3       short body
 *     param $t1       short wid
 *     param $a3       short wpid
 *     param stack+16  int wep
 *     reg   $s2       short wep
 *     reg   $a0       short i
 *     stack sp+16     unsigned char [100] name
 *
 * Globals it touches, as the original declared them:
 *     extern struct WeaponType WeaponDB[28];
 *     extern struct WeaponModelType WeaponModel[41];
 * END PSX.SYM */

static inline void FindWeaponId(Humanoid *human, weapon_kind wid, s16 wpid)
{
    s16 i;

    i = 0;
    while (WeaponDB[i].ilup1.pad != WEAPON_KIND_END)
    {
        if (WeaponDB[i].ilup1.pad == wid)
        {
            human->wepid[wpid] = i;
            break;
        }
        i++;
    }
}

void GetWeaponData(Humanoid *human, model_part_index body, weapon_kind wid,
                   s16 wpid,
                   int wep)
{
    s16 w;
    s16 i;
    u8 name[100];
    OrnamentType *base;

    w = (s16)wep;
    if (wpid >= 0)
    {
        FindWeaponId(human, wid, wpid);
    }

    if (w >= 0)
    {
        i = 0;
        while (WeaponModel[i].wid != WEAPON_KIND_END)
        {
            if (WeaponModel[i].wid == wid)
            {
                break;
            }
            i++;
        }
        if (WeaponModel[i].wid != WEAPON_KIND_END)
        {
            if (WeaponModel[i].model == 0)
            {
                sprintf(name, fmt_tmd, path_human_weapon, WeaponModel[i].name);
                WeaponModel[i].model = FileRead(name);
            }
            base = LoadOrnament(WeaponModel[i].model);
            human->weapon[w] = base;
            GsInitCoordinate2(&human->model->object[body]->locate,
                              &base->locate);
        }
    }
}

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
        GetWeaponData(human, MODEL_PART_TORSO, KEITOUB,
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
        GetWeaponData(human, MODEL_PART_TORSO, human->wpatk + 2,
                      WEAPON_HAND_NONE, WEAPON_SLOT_ACTIVE_1);
        GetWeaponData(human, MODEL_PART_WEAPON_HAND_1, human->wpatk + 1,
                      WEAPON_HAND_NONE, WEAPON_SLOT_INACTIVE_0);
        break;
    default:
        return;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetAttackDBID(struct Humanoid *human, short mid);
 *     APPEAR.C:258, 8 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short mid
 *
 * Globals it touches, as the original declared them:
 *     extern struct BattleType BattleDB[78];
 * END PSX.SYM */

s16 GetAttackDBID(Humanoid *human, motion_id mid)
{
    s16 i;

    mid = GetMotionID(human->motion, mid);
    i = 0;
    while (BattleDB[i].mid != MOTION_ID_NONE)
    {
        if (BattleDB[i].mid == mid)
        {
            break;
        }
        i++;
    }
    return i;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeWeapon(struct Humanoid *human);
 *     APPEAR.C:364, 7 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 * END PSX.SYM */

void DisposeWeapon(Humanoid *human)
{
    OrnamentType **weapons;
    short i;

    weapons = human->weapon;
    i = 0;
    do
    {
        DisposeOrnament(weapons[i]);
        weapons[i] = 0;
        i++;
    } while (i < N_WEAPON_SLOTS);
}

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
