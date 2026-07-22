#ifndef APPEAR_H
#define APPEAR_H

/* APPEAR.C's recovered interface, shared because the original source file is
 * reconstructed as several translation units. */
struct Humanoid;

extern void SetupAppearance(short mode, short stage);
extern struct Humanoid *SetupCharacterParameter(short type,
                                                struct Humanoid *human);
extern struct Humanoid *BreedLife(short type, long x, long y, long z, long r);
extern short GetAttackDBID(struct Humanoid *human, short mid);
extern void GetWeaponData(struct Humanoid *human, short body, short wid,
                          short wpid, int wep);
extern void SetupWeapon(struct Humanoid *human);
extern void DisposeWeapon(struct Humanoid *human);
extern void EquipWeapon(struct Humanoid *human, short mode);

#endif
