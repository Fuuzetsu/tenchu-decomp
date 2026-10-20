#ifndef MENU_TITLE_H
#define MENU_TITLE_H

#include "frontend.h"

enum
{
    TITLE_MESH_CELLS = 16,
    TITLE_MESH_VERTICES = TITLE_MESH_CELLS + 1,
    TITLE_MESH_UPDATE_FRAMES = 5,
    TITLE_MESH_PRIORITY = 1249,
    TITLE_RIPPLE_PHASE_STEP = 250,
    TITLE_RIPPLE_ATTENUATION = 20
};

/* TITLE.ARC's sixteen TIMs, in archive order. The seal strips interleave
 * to form the nine characters around the title. */
enum TitleImageId
{
    TITLE_IMAGE_BACKGROUND,
    TITLE_IMAGE_BLOOD,
    TITLE_IMAGE_SUBTITLE,
    TITLE_IMAGE_IMPACT,
    TITLE_IMAGE_SPARK,
    TITLE_IMAGE_SEALS_EVEN,
    TITLE_IMAGE_SEALS_ODD,
    TITLE_IMAGE_GENRE,
    TITLE_IMAGE_KUNAI,
    TITLE_IMAGE_ROMAN_LOGO,
    TITLE_IMAGE_LOGO,
    TITLE_IMAGE_TRADEMARK,
    TITLE_IMAGE_COPYRIGHT_LEFT,
    TITLE_IMAGE_COPYRIGHT_RIGHT,
    TITLE_IMAGE_MENU_TEXT,
    TITLE_IMAGE_MENU_BACKING
};

enum
{
    TITLE_SEAL_COUNT = 9,
    TITLE_MENU_ITEMS = 4,
    TITLE_PALETTE_COLORS = 16
};

typedef struct TitlePalette
{
    u_short colors[TITLE_PALETTE_COLORS];
} TitlePalette;

extern ArcFile *TitleImageArchive;
extern short TitleMeshX[TITLE_MESH_VERTICES][TITLE_MESH_VERTICES];
extern short TitleMeshY[TITLE_MESH_VERTICES][TITLE_MESH_VERTICES];
extern short TitleMeshPhase[TITLE_MESH_VERTICES][TITLE_MESH_VERTICES];
extern POLY_FT4 TitleMeshPolygons[TITLE_MESH_CELLS * TITLE_MESH_CELLS];

extern GsSPRITE TitleSealSprites[TITLE_SEAL_COUNT];
extern GsSPRITE TitleLogoSprite;
extern GsSPRITE TitleGenreSprite;
extern GsSPRITE TitleSubtitleSprite;
extern GsSPRITE TitleRomanLogoSprite;
extern GsSPRITE TitleKunaiSprite;
extern GsSPRITE TitleTrademarkSprite;
extern GsSPRITE TitleSparkSprite;
extern GsSPRITE TitleCopyrightSprites[2];
extern GsSPRITE TitleImpactSprite;
extern GsSPRITE TitleBloodSprite;
extern GsSPRITE TitleMenuSprites[TITLE_MENU_ITEMS][2];
extern TitlePalette TitleSealPalette;
extern TitlePalette TitleBloodPalette;
extern short TitleBloodColorOrder[TITLE_SEAL_COUNT];

extern short TitleSealClutX, TitleSealClutY;
extern int TitleSealFrame, TitleSealAngle, TitleSealSpeed;
extern short TitleBloodClutX, TitleBloodClutY;
extern short TitleBloodFrame;
extern int TitleLogoFrame, TitleGenreFrame, TitleSubtitleFrame;
extern int TitleKunaiFrame, TitleCopyrightFrame, TitleSelection;
extern int TitlePadRetryFrames;

short RunTitleMenu(void);
void LoadTitleSprites(void);
void DrawTitleSprites(void);
void InitTitleMesh(void);
void UpdateTitleMesh(void);
void DrawTitleMesh(void);

#endif
