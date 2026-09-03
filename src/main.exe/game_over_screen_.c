#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "model.h"
#include "tim.h"
#include "graphics.h"
#include "effect.h"
#include "appear.h"
#include <psxsdk/libgpu.h>
#include "images.h"
#include "vmemory.h"

/* Fade up from black, hold on the title, let the player page to the archive
 * text, then fade back down to either a retry or the menu. */
enum game_over_state
{
    GAMEOVER_FADE_IN = 1,
    GAMEOVER_TITLE = 2,
    GAMEOVER_ARCHIVE = 3,
    GAMEOVER_FADE_TO_RETRY = 4,
    GAMEOVER_FADE_TO_MENU = 5
};

/* Shared expansion for the two identical caption fades. */
#define FADE_IN_LINE(spr, at)                                                 \
    if (GameClock >= at)                                                      \
    {                                                                         \
        increment = spr.b + 1;                                                \
        color = -0x80;                                                        \
        if (increment < 0x80)                                                 \
        {                                                                     \
            color = increment;                                                \
        }                                                                     \
        spr.r = spr.g = spr.b = color;                                        \
        GsSortSprite(&spr, OTablePt, GAME_OVER_TEXT_OT_PRIORITY);                                   \
    }

/* Shared expansion for the two identical archive-text lines. */
#define INIT_ARCHIVE_LINE(spr, idx, y0)                                       \
    tim = get_tim_from_archive(fade_archive, idx);                            \
    StartDemoInitSprite(tim, &image, &spr);                                   \
    spr.y = y0;                                                               \
    spr.x = 0;                                                                \
    spr.r = 0;                                                                \
    spr.g = 0;                                                                \
    spr.b = 0;                                                                \
    spr.attribute |= GS_ATTR_SEMITRANS_ADD;                                   \
    spr.mx = spr.w >> 1;                                                      \
    spr.my = spr.h >> 1;                                                      \
    LoadTIM(tim);

#define RESET_GAME_OVER_TITLE_FADE(state_, shade_)                            \
    do                                                                        \
    {                                                                         \
        (state_) = GAMEOVER_TITLE;                                            \
        (shade_) = 0;                                                         \
    } while (0)

#define ENTER_GAME_OVER_TITLE(state_, shade_, rect_)                          \
    do                                                                        \
    {                                                                         \
        RESET_GAME_OVER_TITLE_FADE(state_, shade_);                           \
        (rect_).x = 0x280;                                                    \
        (rect_).y = 360;                                                      \
        (rect_).w = 0x100;                                                    \
        GameClock = 0;                                                        \
        (rect_).h = 0x28;                                                     \
    } while (0)

#define HANDLE_GAME_OVER_EXIT_INPUT(state_, new_press_)                       \
    do                                                                        \
    {                                                                         \
        if (((new_press_) & PADRright) != 0)                                  \
        {                                                                     \
            (state_) = GAMEOVER_FADE_TO_RETRY;                                \
        }                                                                     \
        if (((new_press_) & PADstart) != 0 ||                                 \
            GameClock >= GAME_OVER_TIMEOUT)                                   \
        {                                                                     \
            (state_) = GAMEOVER_FADE_TO_MENU;                                 \
        }                                                                     \
    } while (0)

extern u8 CHOSEN_CHARACTER;
extern u8 STAGE_LAYOUT_NUMBER;
extern char path_demo_start_fadeio_tim[]; /* K:\\WORK\\CDIMAGE\\DEMO\\start\\fadeio.tim */
extern char fmt_arc[];                    /* %s%s%c.Arc */
extern char path_demo[];                  /* K:\\WORK\\CDIMAGE\\DEMO\\ */
extern BackGround *load_background_(u_long *tim);
/* Retail declares shade as s16 here; tile_sprite_ defines it as u16. */
extern void tile_sprite_(Sprite3D *sprite, s16 shade);
extern void exec_process_(s32 mode);

static inline void StartDemoInitSprite(u_long *tim, GsIMAGE *image,
                                       GsSPRITE *sprite)
{
    GetTIMInfo(tim, image);
    InitSprite(image, sprite);
}

void game_over_screen_(void)
{
    GsIMAGE fade_image;
    GsSPRITE gov_title;
    GsSPRITE archive_line_1;
    GsSPRITE archive_line_2;
    GsSPRITE archive_line_3;
    GsSPRITE gov_prompt;
    RECT clear_rect;
    char archive_path[64];
    GsIMAGE image;
    s16 old_pad;
    BackGround *background;
    ArcFile *gov_archive;
    u_long *tim;
    ArcFile *fade_archive;
    Sprite3D *fade_sprite;
    u8 *persistent;
    TLinkInfo *language_state;
    char *resource_root;
    u16 pad;
    u16 previous_pad;
    u16 new_press;
    s16 shade;
    enum game_over_state state;
    s32 title_brightness;
    s32 setup_brightness;
    u32 color;
    s32 increment;
    s32 i;
    s32 suffix;
    s32 clear_b;
    s32 chr_offset;
    char **prefix_entry;

    state = GAMEOVER_FADE_IN;
    shade = 0x80;
    title_brightness = 0;
    old_pad = 0;
    clear_b = 0;
    SetupAppearance(RIKIMARU_0, APPEARANCE_STAGE_NONE);
    PadShockAR(PAD_PORT_1, RUMBLE_POWER_OFF, RUMBLE_ATTACK_NONE, RUMBLE_RELEASE_NONE);

    i = 0;
    persistent = (u8 *)TENCHU_PERSISTENT_STATE_ADDRESS;
    do
    {
        chr_offset = SAVE_ITEM_ROW_OFFSET(CHOSEN_CHARACTER);
        persistent[TLINKINFO_BYTE_OFFSET(saveItem[0]) + i] =
            persistent[(i + chr_offset) +
                       TLINKINFO_BYTE_OFFSET(gItem[0][0])];
        i++;
    } while (i < N_LOADOUT_ITEMS);

    FadeOutDirect(SCREEN_FADE_FRAMES, SCREEN_FADE_BLEND, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL);
    clear_screen_();
    setRECT(&clear_rect, 0, 0, VRAM_W, VRAM_H);
    ClearImage(&clear_rect, 0, 0, clear_b);
    DrawSync(0);

    tim = FileRead(path_demo_start_fadeio_tim);
    GetTIMInfo(tim, &fade_image);
    LoadTIMAndFree(tim);
    fade_sprite = SetupSprite(0, &fade_image);
    suffix = 'r';
    fade_sprite->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;

    language_state = (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
    if (CHOSEN_CHARACTER != RIKIMARU_0)
    {
        suffix = 'a';
    }
    resource_root = path_demo;
    prefix_entry = &GAME_OVER_FADE_PREFIXES[language_state->language];
    sprintf(archive_path, fmt_arc, resource_root, *prefix_entry, suffix);
    fade_archive = (ArcFile *)FileRead(archive_path);
    tim = get_tim_from_archive(fade_archive, GAME_OVER_FADE_BACKGROUND);
    background = load_background_(tim);
    gov_archive = (ArcFile *)PathFileRead(
        resource_root,
        GAME_OVER_ARCHIVE_PATHS[language_state->language]);
    setup_brightness = 0x80;
    tim = get_tim_from_archive(gov_archive, GAME_OVER_TITLE_IMAGE);
    StartDemoInitSprite(tim, &image, &gov_title);
    gov_title.y = -40;
    gov_title.x = 0;
    gov_title.r = setup_brightness;
    gov_title.g = setup_brightness;
    gov_title.b = setup_brightness;
    gov_title.attribute |= GS_ATTR_SEMITRANS_ADD;
    gov_title.mx = gov_title.w >> 1;
    gov_title.my = gov_title.h >> 1;
    LoadTIM(tim);

    tim = get_tim_from_archive(gov_archive, GAME_OVER_PROMPT_IMAGE);
    StartDemoInitSprite(tim, &image, &gov_prompt);
    increment = gov_prompt.attribute;
    gov_prompt.y = 95;
    gov_prompt.x = 0;
    gov_prompt.r = setup_brightness;
    gov_prompt.g = setup_brightness;
    gov_prompt.b = setup_brightness;
    gov_prompt.attribute = increment;
    gov_prompt.mx = gov_prompt.w >> 1;
    gov_prompt.my = gov_prompt.h >> 1;
    LoadTIM(tim);

    tim = get_tim_from_archive(fade_archive, GAME_OVER_FADE_LINE_1);
    StartDemoInitSprite(tim, &image, &archive_line_1);
    archive_line_1.x = 0;
    archive_line_1.y = 0;
    archive_line_1.r = 0;
    archive_line_1.g = 0;
    archive_line_1.b = 0;
    archive_line_1.attribute |= GS_ATTR_SEMITRANS_ADD;
    archive_line_1.mx = archive_line_1.w >> 1;
    archive_line_1.my = archive_line_1.h >> 1;
    LoadTIM(tim);

    INIT_ARCHIVE_LINE(archive_line_2, GAME_OVER_FADE_LINE_2, 20);

    INIT_ARCHIVE_LINE(archive_line_3, GAME_OVER_FADE_LINE_3, 40);

    DrawSync(0);
    VSync(0);
    _PlayMusic(MUSIC_TRACK_GAMEOVER, CDA_ONCE);

    while (1)
    {
        StartDrawing();
        DrawBG(background);

        switch (state)
        {
        case GAMEOVER_FADE_IN:
            shade -= 2;
            if (shade <= 0)
            {
                ENTER_GAME_OVER_TITLE(state, shade, clear_rect);
            }
            tile_sprite_(fade_sprite, shade);
            break;

        case GAMEOVER_TITLE:
            previous_pad = old_pad;
            pad = GetRealPad(PAD_PORT_1);
            old_pad = pad;
            new_press = pad & (pad ^ previous_pad);
            if ((new_press & PADRright) != 0 && GameClock < 0x23b)
            {
                state = GAMEOVER_ARCHIVE;
                gov_title.r = gov_title.g = gov_title.b = 0x80;
                archive_line_1.r = archive_line_1.g = archive_line_1.b =
                    0x80;
                archive_line_2.r = archive_line_2.g = archive_line_2.b =
                    0x80;
                archive_line_3.r = archive_line_3.g = archive_line_3.b =
                    0x80;
                GsSortSprite(&gov_title, OTablePt, GAME_OVER_TITLE_OT_PRIORITY);
                GsSortSprite(&archive_line_1, OTablePt, GAME_OVER_TEXT_OT_PRIORITY);
                GsSortSprite(&archive_line_2, OTablePt, GAME_OVER_TEXT_OT_PRIORITY);
                GsSortSprite(&archive_line_3, OTablePt, GAME_OVER_TEXT_OT_PRIORITY);
                GsSortSprite(&gov_prompt, OTablePt, GAME_OVER_TEXT_OT_PRIORITY);
                break;
            }

            if (GameClock >= 0x4c)
            {
                title_brightness++;
                if (title_brightness >= 0x80)
                {
                    title_brightness = 0x80;
                }
                gov_title.r = gov_title.g = gov_title.b = title_brightness;
                GsSortSprite(&gov_title, OTablePt, GAME_OVER_TITLE_OT_PRIORITY);
            }
            FADE_IN_LINE(archive_line_1, GAME_OVER_LINE_1_FADE_FRAME)
            FADE_IN_LINE(archive_line_2, GAME_OVER_LINE_2_FADE_FRAME)
            FADE_IN_LINE(archive_line_3, GAME_OVER_LINE_3_FADE_FRAME)
            if (GameClock < 0x23b)
            {
                break;
            }
            goto sort_prompt_and_handle_input;

        case GAMEOVER_ARCHIVE:
            previous_pad = old_pad;
            pad = GetRealPad(PAD_PORT_1);
            old_pad = pad;
            new_press = pad & (pad ^ previous_pad);
            GsSortSprite(&gov_title, OTablePt, GAME_OVER_TITLE_OT_PRIORITY);
            GsSortSprite(&archive_line_1, OTablePt, GAME_OVER_TEXT_OT_PRIORITY);
            GsSortSprite(&archive_line_2, OTablePt, GAME_OVER_TEXT_OT_PRIORITY);
            GsSortSprite(&archive_line_3, OTablePt, GAME_OVER_TEXT_OT_PRIORITY);
        sort_prompt_and_handle_input:
            GsSortSprite(&gov_prompt, OTablePt, GAME_OVER_TEXT_OT_PRIORITY);
            HANDLE_GAME_OVER_EXIT_INPUT(state, new_press);
            break;

        case GAMEOVER_FADE_TO_RETRY:
            shade += 4;
            if (shade >= 0x80)
            {
                GameRetry |= GAME_RETRY_REPLAY;
                vfree(fade_archive);
                vfree(gov_archive);
                vfree(fade_sprite);
                DisposeBG(background);
                exec_process_(PROCESS_MAIN);
            }
            tile_sprite_(fade_sprite, shade);
            break;

        case GAMEOVER_FADE_TO_MENU:
            shade += 4;
            if (shade >= 0x80)
            {
                GameRetry &= (u8)~GAME_RETRY_REPLAY;
                vfree(fade_archive);
                vfree(gov_archive);
                vfree(fade_sprite);
                DisposeBG(background);
                STAGE_LAYOUT_NUMBER = STAGE_LAYOUT_RANDOM;
                exec_process_(PROCESS_MENU);
            }
            tile_sprite_(fade_sprite, shade);
            break;
        }

        SkipFrame = SKIPFRAME_AFTER_LOAD;
        EndDrawing(0);
    }
}
#undef HANDLE_GAME_OVER_EXIT_INPUT
#undef ENTER_GAME_OVER_TITLE
#undef RESET_GAME_OVER_TITLE_FADE
