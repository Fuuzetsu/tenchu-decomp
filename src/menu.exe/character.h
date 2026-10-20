#ifndef MENU_CHARACTER_H
#define MENU_CHARACTER_H

#include "frontend.h"

enum
{
    CHARACTER_SELECTION_SPRITES = 3,
    CHARACTER_SELECTION_WEAPONS = 3,
    CHARACTER_SELECTION_MIRROR_X = 0x00800000,
    /* These IDs use this screen's STAGE9.AMD registrations, not MAIN's
     * gameplay registrations: selected, waiting, and drawing a weapon. */
    CHARACTER_MOTION_SELECTED = 0xF04,
    CHARACTER_MOTION_WAITING = 0x501,
    CHARACTER_MOTION_CONFIRM = 0x80E
};

extern ArcFile *CharacterImageArchive;
extern u_long *CharacterModelData[N_PLAYABLE_CHARACTERS];
extern ModelArchiveType *CharacterModels[N_PLAYABLE_CHARACTERS];
extern u_long *CharacterMotionData;
extern MotionManager *CharacterMotions[N_PLAYABLE_CHARACTERS];
extern u_long *CharacterWeaponData[N_PLAYABLE_CHARACTERS];
extern BackGround *CharacterBackground;
extern OrnamentType *CharacterWeapons[CHARACTER_SELECTION_WEAPONS];
extern Sprite3D *CharacterSprites[CHARACTER_SELECTION_SPRITES];
extern MotionRegistType RikimaruSelectionMotions[9];
extern MotionRegistType AyameSelectionMotions[8];
extern char *CharacterImagePaths[N_LANGUAGES];

short SelectMenuCharacter(void);
void SetupCharacterImages(short dispose);
void SetupCharacterModels(short dispose);

#endif
