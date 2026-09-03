#ifndef APPEAR_H
#define APPEAR_H

/* APPEAR.C's recovered interface. */
struct Humanoid;

/* EquipWeapon's requested visible state. */
enum
{
    WEAPON_SHEATHED = 0,
    WEAPON_DRAWN = 1
};

/* Passing no stage tears down all loaded appearance motion packs. */
enum
{
    APPEARANCE_STAGE_NONE = -1
};

/* APPEAR.C-private originally; externally linked while its storage is raw. */
extern short NowStage;

extern void SetupAppearance(character_kind character, short stage);
extern struct Humanoid *SetupCharacterParameter(character_kind type,
                                                struct Humanoid *human);
extern struct Humanoid *BreedLife(character_kind type, long x, long y,
                                  long z, long r);
extern short GetAttackDBID(struct Humanoid *human, motion_id mid);
extern void GetWeaponData(struct Humanoid *human, model_part_index body,
                          weapon_kind wid, short wpid, int wep);
extern void SetupWeapon(struct Humanoid *human);
extern void DisposeWeapon(struct Humanoid *human);
extern void EquipWeapon(struct Humanoid *human, short mode);

#endif
