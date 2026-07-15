#include "common.h"
#include "main.exe.h"

/*
 * start_demo_ (0x80055d64) — load and run the post-stage demo start screen:
 * fade in its five localized sprites, accept the continue/cancel inputs,
 * then release both archives and dispatch to the selected executable.
 *
 * STATUS: NON_MATCHING — complete guarded pure-C reconstruction. The draft
 * recovers the full five-state jump-table loop, including state 2's shared
 * prompt/input tail with state 3, resource ownership, timed sprite fades,
 * pad-edge detection and both cleanup paths. It has the exact 0x1a0 frame and
 * exact 2188-byte/547-instruction extent; fuzzy score 90.39% (up from the
 * comments-only scaffold's 7.01%, and 71.29% before this pass). Remaining
 * differences are localized compiler allocation/scheduling around the setup
 * calls and sprite scratch image. Build with `NON_MATCHING=start_demo_ ./Build`.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/start_demo_", start_demo_);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/start_demo_", start_demo___override__prt_80055ee4_68447b59);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/start_demo_", switchD_800561a0__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/start_demo_", switchD_800561a0__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/start_demo_", switchD_800561a0__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/start_demo_", switchD_800561a0__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/start_demo_", switchD_800561a0__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/start_demo_", switchD_800561a0__caseD_5);

/* jump-table pool @ 0x80013bb0 (5 words; tables at 0x80013bb0) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 start_demo__jtbl[5] = {
    0x800561A8, 0x800561F8, 0x80056438, 0x800564EC,
    0x80056550,
};

#else /* NON_MATCHING */

typedef struct BackGround BackGround;
typedef struct
{
    u8 pad[0x68];
    GsSPRITE sprite;
} Sprite3D;

typedef struct
{
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} RECT;

extern u8 CHOSEN_CHARACTER;
extern u8 CHOSEN_LANGUAGE;
extern u8 STAGE_LAYOUT_NUMBER;
extern u8 D_80010048;
extern u8 ITEM_LOADOUT_BACKUP[];
extern u8 SHOP_STOCK_STATE_BY_CHAR[];
extern char D_80013AFC[];
extern char D_80013B24[];
extern char D_800137A0[];
extern char *GOV_RESOURCE_PREFIX_PTRS[];
extern char *GOV_ARCHIVE_PTRS[];
extern s32 GameClock;
extern s16 SkipFrame;
extern GsOT *OTablePt;

extern void SetupAppearance(s16 mode, s16 stage);
extern void PadShockAR(s32 port, s32 duration, s32 strength, s32 delay);
extern void FadeOutDirect(s16 time, s16 attrib, u8 r, u8 g, s32 b);
extern void FUN_80038ce0(void);
extern void ClearImage(RECT *rect, u8 r, u8 g, u8 b);
extern s32 DrawSync(s32 mode);
extern s32 VSync(s32 mode);
extern u_long *FileRead(char *path);
extern Sprite3D *SetupSprite(Sprite3D *original, GsIMAGE *image);
extern int sprintf(char *buffer, char *format, ...);
extern u_long *get_tim_from_archive(u_long *archive, int index);
extern BackGround *FUN_8004f4f8(u_long *tim);
extern void InitSprite(GsIMAGE *image, GsSPRITE *sprite);
extern void LoadTIM(u_long *tim);
extern void _PlayMusic(s32 music, s32 mode);
extern void StartDrawing(void);
extern short DrawBG(BackGround *background);
extern s32 GetRealPad(s32 port);
extern void GsSortSprite(GsSPRITE *sprite, GsOT *ot, s32 priority);
extern void FUN_80056910(Sprite3D *sprite, s16 shade);
extern void vfree(void *ptr);
extern void DisposeBG(BackGround *background);
extern void FUN_8004f6c0(s32 mode);
extern void EndDrawing(s16 mode);

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
    volatile u16 old_pad;
    BackGround *background;
    u_long *gov_archive;
    u_long *tim;
    u_long *fade_archive;
    Sprite3D *fade_sprite;
    u8 *persistent;
    u8 *language_state;
    char *resource_root;
    u16 pad;
    u16 previous_pad;
    u16 new_press;
    s16 shade;
    s32 state;
    s32 title_brightness;
    s32 full_brightness;
    s32 color;
    s32 increment;
    s32 i;
    s32 suffix;

    state = 1;
    shade = 0x80;
    title_brightness = 0;
    old_pad = 0;
    SetupAppearance(0, -1);
    PadShockAR(0, 0, 0, 0);

    i = 0;
    persistent = (u8 *)0x80010000;
    do {
        persistent[0x27 + i] =
            persistent[i + CHOSEN_CHARACTER * 0x20 + 0x40c];
        i++;
    } while (i < 0x14);

    FadeOutDirect(0x20, 2, 8, 8, 8);
    FUN_80038ce0();
    clear_rect.x = 0;
    clear_rect.y = 0;
    clear_rect.w = 0x400;
    clear_rect.h = 0x200;
    ClearImage(&clear_rect, 0, 0, 0);
    DrawSync(0);

    tim = FileRead(D_80013AFC);
    GetTIMInfo(tim, &fade_image);
    LoadTIMAndFree(tim);
    fade_sprite = SetupSprite(0, &fade_image);
    suffix = 'r';
    fade_sprite->sprite.attribute |= 0x60000000;

    language_state = (u8 *)0x80010000;
    if (CHOSEN_CHARACTER != 0)
    {
        suffix = 'a';
    }
    resource_root = D_800137A0;
    sprintf(archive_path, D_80013B24, resource_root,
            GOV_RESOURCE_PREFIX_PTRS[language_state[0x5e]], suffix);
    full_brightness = 0x80;
    fade_archive = FileRead(archive_path);
    tim = get_tim_from_archive(fade_archive, 0);
    background = FUN_8004f4f8(tim);
    gov_archive = PathFileRead(resource_root,
                               GOV_ARCHIVE_PTRS[language_state[0x5e]]);

    tim = get_tim_from_archive(gov_archive, 0);
    GetTIMInfo(tim, &image);
    InitSprite(&image, &gov_title);
    gov_title.y = -0x28;
    gov_title.x = 0;
    gov_title.r = 0x80;
    gov_title.g = 0x80;
    gov_title.b = 0x80;
    gov_title.attribute |= 0x50000000;
    gov_title.my = gov_title.h >> 1;
    gov_title.mx = gov_title.w >> 1;
    LoadTIM(tim);

    tim = get_tim_from_archive(gov_archive, 1);
    GetTIMInfo(tim, &image);
    InitSprite(&image, &gov_prompt);
    gov_prompt.y = 0x5f;
    gov_prompt.x = 0;
    gov_prompt.r = 0x80;
    gov_prompt.g = 0x80;
    gov_prompt.b = 0x80;
    gov_prompt.mx = gov_prompt.w >> 1;
    gov_prompt.my = gov_prompt.h >> 1;
    LoadTIM(tim);

    tim = get_tim_from_archive(fade_archive, 1);
    GetTIMInfo(tim, &image);
    InitSprite(&image, &archive_line_1);
    archive_line_1.x = 0;
    archive_line_1.y = 0;
    archive_line_1.r = 0;
    archive_line_1.g = 0;
    archive_line_1.b = 0;
    archive_line_1.attribute |= 0x50000000;
    archive_line_1.my = archive_line_1.h >> 1;
    archive_line_1.mx = archive_line_1.w >> 1;
    LoadTIM(tim);

    tim = get_tim_from_archive(fade_archive, 2);
    GetTIMInfo(tim, &image);
    InitSprite(&image, &archive_line_2);
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
    GetTIMInfo(tim, &image);
    InitSprite(&image, &archive_line_3);
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
            do {
                do {
                    do {
                        do {
                            shade -= 2;
                        } while (0);
                    } while (0);
                } while (0);
            } while (0);
            if (shade <= 0)
            {
                do {
                    do {
                        do {
                            do {
                                state = 2;
                            } while (0);
                        } while (0);
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
            do {
                pad = GetRealPad(0);
            } while (0);
            old_pad = pad;
            new_press = pad & (pad ^ previous_pad);
            if ((new_press & 0x20) != 0 && GameClock < 0x23b)
            {
                state = 3;
                gov_title.r = gov_title.g = gov_title.b = full_brightness;
                archive_line_1.r = archive_line_1.g = archive_line_1.b =
                    full_brightness;
                archive_line_2.r = archive_line_2.g = archive_line_2.b =
                    full_brightness;
                archive_line_3.r = archive_line_3.g = archive_line_3.b =
                    full_brightness;
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
                state = 4;
            }
            if ((new_press & 0x800) != 0 || GameClock >= 0xa8c)
            {
                state = 5;
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

#endif /* NON_MATCHING */
