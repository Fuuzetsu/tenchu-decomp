#ifndef APPEAR_H
#define APPEAR_H

/* APPEAR.C's recovered interface, shared because the original source file is
 * reconstructed as several translation units. */
struct Humanoid;

/* EquipWeapon's requested visible state. */
enum
{
    WEAPON_SHEATHED = 0,
    WEAPON_DRAWN = 1
};

/* APPEAR.C-private originally; extern because that source is split here. */
extern short NowStage;

extern void SetupAppearance(short mode, short stage);
extern struct Humanoid *SetupCharacterParameter(character_kind type,
                                                struct Humanoid *human);
extern struct Humanoid *BreedLife(character_kind type, long x, long y,
                                  long z, long r);
extern short GetAttackDBID(struct Humanoid *human, motion_id mid);
extern void GetWeaponData(struct Humanoid *human, short body,
                          weapon_kind wid, short wpid, int wep);
extern void SetupWeapon(struct Humanoid *human);
extern void DisposeWeapon(struct Humanoid *human);
extern void EquipWeapon(struct Humanoid *human, short mode);

#endif
