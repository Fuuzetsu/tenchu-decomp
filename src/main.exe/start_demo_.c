#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include <psxsdk/libgpu.h>

/*
 * start_demo_ (0x80055d64) loads and runs the localized post-stage demo
 * screen, fades in its sprites, handles continue/cancel input, then releases
 * the resources and dispatches to the selected executable.
 *
 * STATUS: MATCHING — pure C, all 2188 bytes (547 instructions) exact.
 *
 * Naming the selected resource-prefix table entry (rather than only its
 * value) gives GCC 2.8.1 the target local-allocation and sprintf argument
 * schedule. The setup brightness and the later literal 0x80 intentionally
 * remain separate source values; GCC hoists the latter into its own saved
 * register. The dead prompt-attribute read is assigned to `increment`, which
 * is overwritten before any use, reproducing retail's retained v1 load.
 *
 * The zero-trip wrappers on three state transitions are load-bearing compiler
 * shape, plausibly left by statement-macro expansion: their four extra loop
 * depths raise the state allocno from 11 to 15 weighted references (priority
 * 939 versus the fade sprite's 930). Plain assignments rotate s6/s7/fp even
 * though the emitted transition instructions are otherwise identical.
 */

extern u8 CHOSEN_CHARACTER;
extern u8 STAGE_LAYOUT_NUMBER;
extern u8 D_80010048;
extern char D_80013AFC[];
extern char D_80013B24[];
extern char D_800137A0[];
extern char *GOV_RESOURCE_PREFIX_PTRS[];
extern char *GOV_ARCHIVE_PTRS[];

extern void PadShockAR(s32 port, s32 duration, s32 strength, s32 delay);
extern void FadeOutDirect(s16 time, s16 attrib, u8 r, u8 g, s32 b);
extern void FUN_80038ce0(void);
extern s32 VSync(s32 mode);
extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);
extern int sprintf(char *buffer, char *format, ...);
extern BackGround *FUN_8004f4f8(u_long *tim);
extern void LoadTIM(unsigned long *adr);
extern void _PlayMusic(s32 music, s32 mode);
extern short DrawBG(BackGround *bg);
extern s32 GetRealPad(s32 port);
extern void FUN_80056910(Sprite3D *sprite, s16 shade);
extern void vfree(void *ptr);
extern void DisposeBG(BackGround *background);
extern void FUN_8004f6c0(s32 mode);

static inline void StartDemoInitSprite(u_long *tim, GsIMAGE *image,
                                       GsSPRITE *sprite)
{
    GetTIMInfo(tim, image);
    InitSprite(image, sprite);
}

void start_demo_(void)
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
    u_long *gov_archive;
    u_long *tim;
    u_long *fade_archive;
    Sprite3D *fade_sprite;
    u8 *persistent;
    TLinkInfo *language_state;
    char *resource_root;
    u16 pad;
    u16 previous_pad;
    u16 new_press;
    s16 shade;
    s32 state;
    s32 title_brightness;
    s32 setup_brightness;
    u32 color;
    s32 increment;
    s32 i;
    s32 suffix;
    s32 clear_b;
    s32 chr_offset;
    char **prefix_entry;

    state = 1;
    shade = 0x80;
    title_brightness = 0;
    old_pad = 0;
    clear_b = 0;
    SetupAppearance(0, -1);
    PadShockAR(0, 0, 0, 0);

    i = 0;
    persistent = (u8 *)TENCHU_PERSISTENT_STATE_ADDRESS;
    do {
        chr_offset = CHOSEN_CHARACTER * 0x20;
        persistent[0x27 + i] = persistent[(i + chr_offset) + 0x40c];
        i++;
    } while (i < 0x14);

    FadeOutDirect(0x20, 2, 8, 8, 8);
    FUN_80038ce0();
    clear_rect.x = 0;
    clear_rect.y = 0;
    clear_rect.w = 0x400;
    clear_rect.h = 0x200;
    ClearImage(&clear_rect, 0, 0, clear_b);
    DrawSync(0);

    tim = FileRead(D_80013AFC);
    GetTIMInfo(tim, &fade_image);
    LoadTIMAndFree(tim);
    fade_sprite = SetupSprite(0, &fade_image);
    suffix = 'r';
    fade_sprite->sprite.attribute |= 0x60000000;

    language_state = (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
    if (CHOSEN_CHARACTER != 0)
    {
        suffix = 'a';
    }
    resource_root = D_800137A0;
    prefix_entry = &GOV_RESOURCE_PREFIX_PTRS[language_state->language];
    sprintf(archive_path, D_80013B24, resource_root, *prefix_entry, suffix);
    fade_archive = FileRead(archive_path);
    tim = get_tim_from_archive(fade_archive, 0);
    background = FUN_8004f4f8(tim);
    gov_archive = PathFileRead(resource_root,
                               GOV_ARCHIVE_PTRS[language_state->language]);
    setup_brightness = 0x80;
    tim = get_tim_from_archive(gov_archive, 0);
    StartDemoInitSprite(tim, &image, &gov_title);
    gov_title.y = -0x28;
    gov_title.x = 0;
    gov_title.r = setup_brightness;
    gov_title.g = setup_brightness;
    gov_title.b = setup_brightness;
    gov_title.attribute |= 0x50000000;
    gov_title.mx = gov_title.w >> 1;
    gov_title.my = gov_title.h >> 1;
    LoadTIM(tim);

    tim = get_tim_from_archive(gov_archive, 1);
    StartDemoInitSprite(tim, &image, &gov_prompt);
    increment = *(volatile u32 *)&gov_prompt.attribute;
    gov_prompt.y = 0x5f;
    gov_prompt.x = 0;
    gov_prompt.r = setup_brightness;
    gov_prompt.g = setup_brightness;
    gov_prompt.b = setup_brightness;
    gov_prompt.mx = gov_prompt.w >> 1;
    gov_prompt.my = gov_prompt.h >> 1;
    LoadTIM(tim);

    tim = get_tim_from_archive(fade_archive, 1);
    StartDemoInitSprite(tim, &image, &archive_line_1);
    archive_line_1.x = 0;
    archive_line_1.y = 0;
    archive_line_1.r = 0;
    archive_line_1.g = 0;
    archive_line_1.b = 0;
    archive_line_1.attribute |= 0x50000000;
    archive_line_1.mx = archive_line_1.w >> 1;
    archive_line_1.my = archive_line_1.h >> 1;
    LoadTIM(tim);

    tim = get_tim_from_archive(fade_archive, 2);
    StartDemoInitSprite(tim, &image, &archive_line_2);
    archive_line_2.y = 0x14;
    archive_line_2.x = 0;
    archive_line_2.r = 0;
    archive_line_2.g = 0;
    archive_line_2.b = 0;
    archive_line_2.attribute |= 0x50000000;
    archive_line_2.mx = archive_line_2.w >> 1;
    archive_line_2.my = archive_line_2.h >> 1;
    LoadTIM(tim);

    tim = get_tim_from_archive(fade_archive, 3);
    StartDemoInitSprite(tim, &image, &archive_line_3);
    archive_line_3.y = 0x28;
    archive_line_3.x = 0;
    archive_line_3.r = 0;
    archive_line_3.g = 0;
    archive_line_3.b = 0;
    archive_line_3.attribute |= 0x50000000;
    archive_line_3.mx = archive_line_3.w >> 1;
    archive_line_3.my = archive_line_3.h >> 1;
    LoadTIM(tim);

    DrawSync(0);
    VSync(0);
    _PlayMusic(0xb, 0);

    while (1)
    {
        StartDrawing();
        DrawBG(background);

        switch (state)
        {
        case 1:
            shade -= 2;
            if (shade <= 0)
            {
                do {
                    do {
                        state = 2;
                    } while (0);
                } while (0);
                shade = 0;
                clear_rect.x = 0x280;
                clear_rect.y = 0x168;
                clear_rect.w = 0x100;
                GameClock = 0;
                clear_rect.h = 0x28;
            }
            FUN_80056910(fade_sprite, shade);
            break;

        case 2:
            previous_pad = old_pad;
            pad = GetRealPad(0);
            old_pad = pad;
            new_press = pad & (pad ^ previous_pad);
            if ((new_press & 0x20) != 0 && GameClock < 0x23b)
            {
                state = 3;
                gov_title.r = gov_title.g = gov_title.b = 0x80;
                archive_line_1.r = archive_line_1.g = archive_line_1.b =
                    0x80;
                archive_line_2.r = archive_line_2.g = archive_line_2.b =
                    0x80;
                archive_line_3.r = archive_line_3.g = archive_line_3.b =
                    0x80;
                GsSortSprite(&gov_title, OTablePt, 0xa);
                GsSortSprite(&archive_line_1, OTablePt, 0x50);
                GsSortSprite(&archive_line_2, OTablePt, 0x50);
                GsSortSprite(&archive_line_3, OTablePt, 0x50);
                GsSortSprite(&gov_prompt, OTablePt, 0x50);
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
                GsSortSprite(&gov_title, OTablePt, 0xa);
            }
            if (GameClock >= 0x119)
            {
                increment = archive_line_1.b + 1;
                color = -0x80;
                if (increment < 0x80)
                {
                    color = increment;
                }
                archive_line_1.r = archive_line_1.g = archive_line_1.b = color;
                GsSortSprite(&archive_line_1, OTablePt, 0x50);
            }
            if (GameClock >= 0x15f)
            {
                increment = archive_line_2.b + 1;
                color = -0x80;
                if (increment < 0x80)
                {
                    color = increment;
                }
                archive_line_2.r = archive_line_2.g = archive_line_2.b = color;
                GsSortSprite(&archive_line_2, OTablePt, 0x50);
            }
            if (GameClock >= 0x1a5)
            {
                increment = archive_line_3.b + 1;
                color = -0x80;
                if (increment < 0x80)
                {
                    color = increment;
                }
                archive_line_3.r = archive_line_3.g = archive_line_3.b = color;
                GsSortSprite(&archive_line_3, OTablePt, 0x50);
            }
            if (GameClock < 0x23b)
            {
                break;
            }
            goto sort_prompt_and_handle_input;

        case 3:
            previous_pad = old_pad;
            pad = GetRealPad(0);
            old_pad = pad;
            new_press = pad & (pad ^ previous_pad);
            GsSortSprite(&gov_title, OTablePt, 0xa);
            GsSortSprite(&archive_line_1, OTablePt, 0x50);
            GsSortSprite(&archive_line_2, OTablePt, 0x50);
            GsSortSprite(&archive_line_3, OTablePt, 0x50);
        sort_prompt_and_handle_input:
            GsSortSprite(&gov_prompt, OTablePt, 0x50);
            if ((new_press & 0x20) != 0)
            {
                do {
                    state = 4;
                } while (0);
            }
            if ((new_press & 0x800) != 0 || GameClock >= 0xa8c)
            {
                do {
                    state = 5;
                } while (0);
            }
            break;

        case 4:
            shade += 4;
            if (shade >= 0x80)
            {
                D_80010048 |= 1;
                vfree(fade_archive);
                vfree(gov_archive);
                vfree(fade_sprite);
                DisposeBG(background);
                FUN_8004f6c0(0x11);
            }
            FUN_80056910(fade_sprite, shade);
            break;

        case 5:
            shade += 4;
            if (shade >= 0x80)
            {
                D_80010048 &= 0xfe;
                vfree(fade_archive);
                vfree(gov_archive);
                vfree(fade_sprite);
                DisposeBG(background);
                STAGE_LAYOUT_NUMBER = 0xff;
                FUN_8004f6c0(0x10);
            }
            FUN_80056910(fade_sprite, shade);
            break;
        }

        SkipFrame = 2;
        EndDrawing(0);
    }
}
