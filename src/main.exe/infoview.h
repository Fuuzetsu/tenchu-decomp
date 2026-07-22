#ifndef TENCHU_INFOVIEW_H
#define TENCHU_INFOVIEW_H

/* Retail backing objects for INFOVIEW.C's fixed local menu initializers.
 * TAdtSelect and the surviving local names/bounds come from PSX.SYM; retail
 * data and copy extents determine the final bounds. This follows the other
 * subsystem headers in being included after main.exe.h, where TAdtSelect is
 * defined. */
extern TAdtSelect DEBUG_MENU_ITEM_CHOICE_OPTIONS[25];
extern TAdtSelect D_800124CC[4];
extern TAdtSelect DEBUG_MENU_MAIN_SCREEN_OPTIONS[11];
extern TAdtSelect DEBUG_MENU_ITEM_LAYOUT_OPTIONS[5];
extern TAdtSelect D_8001252C[3];
extern TAdtSelect DEBUG_MENU_HIDDEN_EFFECT_SPAWN_OPTIONS[31];
extern TAdtSelect DEBUG_MENU_ENEMY_LAYOUT_OPTIONS[11];
extern TAdtSelect DEBUG_MENU_ENEMY_PATH_SETTING_OPTIONS[7];
extern TAdtSelect D_800140A8[3];
extern TAdtSelect DEBUG_MENU_LANGUAGE_CHOICES[5];
extern TAdtSelect D_800141F4[3];
extern TAdtSelect DEBUG_MENU_FILE_CHOICES[20];
extern TAdtSelect DEBUG_MENU_SAVE_LOAD_CHOICES[5];
extern TAdtSelect DEBUG_MENU_FILE_LAYOUT_CHOICES[18];
extern TAdtSelect DEBUG_MENU_FILE_LOAD_STOCK_LAYOUT_CHOICES[5];
extern TAdtSelect DEBUG_MENU_STAGE_OPTIONS[11];
extern TAdtSelect DEBUG_MENU_PLAYER_CHOICE_OPTIONS[7];

#endif
