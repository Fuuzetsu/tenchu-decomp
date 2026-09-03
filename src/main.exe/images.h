#ifndef TENCHU_IMAGES_H
#define TENCHU_IMAGES_H

/* LIBGS attributes encode a TIM pixel mode in bits 24-25. For sprites,
 * bits 28-29 carry gpu_blend_mode and bit 30 enables semi-transparency.
 * GsSortSprite translates those fields into the GPU packet encoding. */
enum
{
    GS_ATTR_TEXTURE_MODE_SHIFT = 24,
    GS_ATTR_BLEND_MODE_SHIFT = 28
};

#define GS_ATTR_TEXTURE_MODE(mode) ((mode) << GS_ATTR_TEXTURE_MODE_SHIFT)
#define GS_ATTR_BLEND_MODE(mode) ((mode) << GS_ATTR_BLEND_MODE_SHIFT)
#define GS_ATTR_BLEND_MODE_MASK GS_ATTR_BLEND_MODE(GPU_BLEND_MODE_MASK)
#define GS_ATTR_SEMITRANS_ENABLE 0x40000000
#define GS_ATTR_SEMITRANS_MASK \
    (GS_ATTR_SEMITRANS_ENABLE | GS_ATTR_BLEND_MODE_MASK)
#define GS_ATTR_SEMITRANS(mode) \
    (GS_ATTR_SEMITRANS_ENABLE | GS_ATTR_BLEND_MODE(mode))

#define GS_ATTR_SEMITRANS_AVERAGE GS_ATTR_SEMITRANS(GPU_BLEND_AVERAGE)
#define GS_ATTR_SEMITRANS_ADD GS_ATTR_SEMITRANS(GPU_BLEND_ADD)
#define GS_ATTR_SEMITRANS_SUBTRACT GS_ATTR_SEMITRANS(GPU_BLEND_SUBTRACT)

/* IMAGES.C's original CD location lead-in. */
enum
{
    OFFSET = 150
};

/* ITEMHEL*.ARC contains one help card for every selectable item after the
 * grappling hook, followed by the two selection-limit messages.  All four
 * localized archives have the same 21-entry catalog; the final English TIMs
 * read "Can't select any more items." and
 * "Only four kinds of items can be carried." respectively. */
typedef enum ItemHelpImageId
{
    ITEM_HELP_NONE = -1,
    ITEM_HELP_SHURIKEN = ITEM_SHURIKEN - ITEM_SHURIKEN,
    ITEM_HELP_MAKIBISHI = ITEM_MAKIBISHI - ITEM_SHURIKEN,
    ITEM_HELP_KUSURI = ITEM_KUSURI - ITEM_SHURIKEN,
    ITEM_HELP_FIRE = ITEM_FIRE - ITEM_SHURIKEN,
    ITEM_HELP_SMOKE = ITEM_SMOKE - ITEM_SHURIKEN,
    ITEM_HELP_JIRAI = ITEM_JIRAI - ITEM_SHURIKEN,
    ITEM_HELP_DOKUDANGO = ITEM_DOKUDANGO - ITEM_SHURIKEN,
    ITEM_HELP_GOSHIKIMAI = ITEM_GOSHIKIMAI - ITEM_SHURIKEN,
    ITEM_HELP_NEMURI = ITEM_NEMURI - ITEM_SHURIKEN,
    ITEM_HELP_KAWARIMI = ITEM_KAWARIMI - ITEM_SHURIKEN,
    ITEM_HELP_HENSHIN = ITEM_HENSHIN - ITEM_SHURIKEN,
    ITEM_HELP_GOSIN = ITEM_GOSIN - ITEM_SHURIKEN,
    ITEM_HELP_SHINSOKU = ITEM_SHINSOKU - ITEM_SHURIKEN,
    ITEM_HELP_NINGYO = ITEM_NINGYO - ITEM_SHURIKEN,
    ITEM_HELP_HAPPOU = ITEM_HAPPOU - ITEM_SHURIKEN,
    ITEM_HELP_NINKEN = ITEM_NINKEN - ITEM_SHURIKEN,
    ITEM_HELP_KAENGEKI = ITEM_KAENGEKI - ITEM_SHURIKEN,
    ITEM_HELP_MANEBUE = ITEM_MANEBUE - ITEM_SHURIKEN,
    ITEM_HELP_ARMOUR = ITEM_ARMOUR - ITEM_SHURIKEN,
    ITEM_HELP_ITEM_LIMIT_REACHED = ITEM_ARMOUR,
    ITEM_HELP_KIND_LIMIT_REACHED = ITEM_GUN,
    N_ITEM_HELP_IMAGES = ITEM_ARROW
} ItemHelpImageId;

#define ITEM_HELP_FOR_ITEM(item) \
    ((ItemHelpImageId)((item) - ITEM_SHURIKEN))

/* RANKS*.ARC is shared by the stage-result and leaderboard screens: five
 * rank emblems in stage_rank order, then one portrait for each player. */
typedef enum RankArchiveImageId
{
    RANK_ARCHIVE_THUG = RANK_THUG,
    RANK_ARCHIVE_NOVICE = RANK_NOVICE,
    RANK_ARCHIVE_NINJA = RANK_NINJA,
    RANK_ARCHIVE_MASTER_NINJA = RANK_MASTER_NINJA,
    RANK_ARCHIVE_GRAND_MASTER = RANK_GRAND_MASTER,
    RANK_ARCHIVE_RIKIMARU = N_STAGE_RANKS + RIKIMARU_0,
    RANK_ARCHIVE_AYAME = N_STAGE_RANKS + AYAME_0,
    N_RANK_ARCHIVE_IMAGES = N_STAGE_RANKS + N_PLAYABLE_CHARACTERS
} RankArchiveImageId;

/* Gov_*.Arc supplies the fixed title and prompt.  The character-specific
 * Gov_*_[ra].Arc supplies a background followed by three caption lines. */
typedef enum GameOverImageId
{
    GAME_OVER_TITLE_IMAGE = 0,
    GAME_OVER_PROMPT_IMAGE = 1,
    N_GAME_OVER_IMAGES
} GameOverImageId;

typedef enum GameOverFadeImageId
{
    GAME_OVER_FADE_BACKGROUND = 0,
    GAME_OVER_FADE_LINE_1 = 1,
    GAME_OVER_FADE_LINE_2 = 2,
    GAME_OVER_FADE_LINE_3 = 3,
    N_GAME_OVER_FADE_IMAGES
} GameOverFadeImageId;

/* Complete localized screen-resource tables from IMAGES.C's retail data. */
extern char *ITEM_SELECTION_SCREEN_PATHS[N_LANGUAGES];
extern char *ITEM_HELP_ARCHIVE_PATHS[N_LANGUAGES];
extern char *STAGE_RESULT_BACKGROUND_PATHS[N_LANGUAGES];
extern char *STAGE_RESULT_RANK_ARCHIVE_PATHS[N_LANGUAGES];
extern char *MISSION_SCORE_RANK_ARCHIVE_PATHS[N_LANGUAGES];
extern char *MISSION_SCORE_BACKGROUND_PATHS[N_LANGUAGES];
extern char *GAME_OVER_FADE_PREFIXES[N_LANGUAGES];
extern char *GAME_OVER_ARCHIVE_PATHS[N_LANGUAGES];

extern GsIMAGE *GetImage(ImageArchiveId id);
extern void SetupImageToPolyFT4(GsIMAGE *image, POLY_FT4 *ply,
                                short x, short y);
extern void SetupImageToPolyGT4(GsIMAGE *image, POLY_GT4 *ply,
                                int x, int y);

#endif
