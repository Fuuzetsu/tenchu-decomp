#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "model.h"
#include "tim.h"
#include "graphics.h"
#include "effect.h"
#include <psxsdk/libgpu.h>
#include <psxsdk/libcd.h>
#include "images.h"
#include "vmemory.h"

typedef struct
{
    char *background;
    char *foreground;
    u16 narration_cue; /* _PlayMusic's INTRO.XA cue namespace */
} DemoScreenAssets;

#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

extern u8 CHOSEN_CHARACTER;
extern u8 STAGE_LAYOUT_NUMBER;
extern char path_demo[]; /* K:\\WORK\\CDIMAGE\\DEMO\\ */
/* The adjacent retail symbols prove four complete StageConfig language rows. */
extern DemoScreenAssets BriefingAssets[N_LANGUAGES][N_STAGE_CONFIGS];
extern s16 BriefingLimit[N_LANGUAGES][N_STAGE_CONFIGS];
extern s16 StageScrollAdj[N_LANGUAGES][N_STAGE_CONFIGS];

extern BackGround *load_background_(u_long *tim);
extern void exec_process_(s32 arg0);
extern s32 CdaGetCurrentLength(void);
extern void draw_shade_quad_(void *ot, s32 r, s32 g, s32 b);

static inline void TimToDemoSprite(u_long *file, GsIMAGE *image,
                                   GsSPRITE *sprite)
{
    GetTIMInfo(file, image);
    InitSprite(image, sprite);
}

void briefing_screen_(void)
{
    GsIMAGE strip_image;
    GsSPRITE sprite;
    GsIMAGE image;
    u16 xbase;
    s32 scroll;
    u16 old_pad;
    BackGround *background;
    s32 tpage_base;
    u_long *file;
    u16 pad;
    u16 previous_pad;
    s16 fade;
    s16 counter;
    s32 fade_now;
    enum
    {
        BRIEFING_START_MUSIC = 0,
        BRIEFING_WAIT_AUDIO = 1,
        BRIEFING_HOLD = 2,
        BRIEFING_SCROLL_TEXT = 3
    };
    s16 sequence;
    s32 fade_step;
    s16 strip_width;
    u16 strip_px;
    s32 intensity;
    u8 brightness;
    s32 left_brightness;
    s32 scaled_left_brightness;
    s32 right_brightness;
    s32 scaled_right_brightness;
    s16 i;

    file = PathFileRead((u8 *)path_demo,
                        BriefingAssets[PSTATE->language][PSTATE->StageNo].background);
    sequence = BRIEFING_START_MUSIC;
    fade = 0xfe;
    scroll = -0xa000;
    old_pad = 0;
    xbase = (u16)(scroll >> 8);
    fade_step = -8;
    background = load_background_(file);
    vfree(file);

    file = PathFileRead((u8 *)path_demo,
                        BriefingAssets[PSTATE->language][PSTATE->StageNo].foreground);
    TimToDemoSprite(file, &image, &sprite);
    sprite.x = -160;
    sprite.y = -120;
    sprite.r = 0x80;
    sprite.g = 0x80;
    sprite.b = 0x80;
    sprite.attribute |= GS_ATTR_SEMITRANS_ADD;
    sprite.mx = sprite.w >> 1;
    sprite.my = sprite.h >> 1;
    sprite.mx = 0;
    sprite.my = 0;
    GetTIMInfo(file, &strip_image);
    LoadTIMAndFree(file);
    strip_width = strip_image.pw;
    strip_px = (u16)strip_image.px;
    sprite.w = 0x10;
    sprite.y = -104;
    clear_screen_();
    tpage_base = (s32)strip_px << 16;

    while (1)
    {
        previous_pad = old_pad;
        pad = GetRealPad(PAD_PORT_1);
        old_pad = pad;
        if ((pad & (pad ^ previous_pad) & (PADstart | PADRright)) != 0)
        {
            fade_step = 8;
        }

        if ((pad & (PADstart | PADselect)) == (PADstart | PADselect))
        {
            for (i = 0; i < N_LOADOUT_ITEMS; i++)
            {
                PSTATE->gItem[CHOSEN_CHARACTER][i] =
                    PSTATE->saveItem[i];
            }
            FadeOutDirect(SCREEN_FADE_FRAMES, SCREEN_FADE_BLEND, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL);
            clear_screen_();
            STAGE_LAYOUT_NUMBER = STAGE_LAYOUT_RANDOM;
            GameRetry &= (u8)~GAME_RETRY_REPLAY;
            exec_process_(PROCESS_MENU);
        }

        fade_now = fade;
        if (fade_now >= 0xff)
        {
            break;
        }

        StartDrawing();
        switch (sequence)
        {
        case BRIEFING_START_MUSIC:
            if (fade_now == 0)
            {
                s16 narration_cue;

                narration_cue =
                    BriefingAssets[PSTATE->language][PSTATE->StageNo]
                        .narration_cue;
                if (PSTATE->CharType == AYAME_0 && PSTATE->language == LANG_JAPANESE &&
                    (u32)(PSTATE->StageNo - STAGE_ID_RECLAIM_CASTLE) <=
                        STAGE_ID_FREE_PRINCESS - STAGE_ID_RECLAIM_CASTLE /* the && spelling double-reads the field */)
                {
                    narration_cue++;
                }
                _PlayMusic(narration_cue, CDA_ONCE);
                sequence = BRIEFING_WAIT_AUDIO;
            }
            break;

        case BRIEFING_WAIT_AUDIO:
            if (CdaGetCurrentLength() > 0)
            {
                sequence = BRIEFING_HOLD;
                counter = 0;
            }
            break;

        case BRIEFING_HOLD:
            if (BriefingLimit[PSTATE->language][PSTATE->StageNo] < counter++)
            {
                sequence = BRIEFING_SCROLL_TEXT;
            }
            break;

        case BRIEFING_SCROLL_TEXT:
            counter = strip_width - 4;
            if (counter >= 0)
            {
                s32 renderer_tpage = tpage_base >> 16;
                s32 renderer_width = strip_width;
                s32 renderer_offset;
                s32 renderer_x;
                s16 renderer_raw_x;
                do
                {
                    sprite.u = counter << 2;
                    sprite.tpage = GetTPage(0, 0,
                                            renderer_tpage + counter, 0x100);
                    renderer_offset = renderer_width - counter;
                    renderer_offset <<= 3;
                    renderer_raw_x = xbase - renderer_offset;
                    sprite.x = renderer_raw_x;
                    renderer_x = renderer_raw_x;
                    if (renderer_x < -(SCREEN_W / 2))
                    {
                        goto brightness_zero;
                    }
                    if (renderer_x < -120)
                    {
                        left_brightness = renderer_x + SCREEN_W / 2;
                        scaled_left_brightness = left_brightness;
                        scaled_left_brightness <<= 1;
                        scaled_left_brightness += left_brightness;
                        brightness = scaled_left_brightness;
                        goto brightness_left_store;
                    }
                    if (renderer_x > SCREEN_W / 2)
                    {
                    brightness_zero:
                        sprite.r = 0;
                        sprite.g = 0;
                        sprite.b = 0;
                        goto brightness_done;
                    }
                    if (renderer_x > 120)
                    {
                        right_brightness = SCREEN_W / 2 - renderer_x;
                        scaled_right_brightness = right_brightness;
                        scaled_right_brightness <<= 1;
                        scaled_right_brightness += right_brightness;
                        brightness = scaled_right_brightness;
                    }
                    else
                    {
                        brightness = 0x80;
                    }
                    goto brightness_right_store;

                brightness_left_store:
                    sprite.r = brightness;
                    sprite.g = brightness;
                    sprite.b = brightness;
                    goto brightness_done;

                brightness_right_store:
                    sprite.r = brightness;
                    sprite.g = brightness;
                    sprite.b = brightness;
                brightness_done:
                    sprite.x -= 8;
                    GsSortSprite(&sprite, OTablePt, 1);
                    counter -= 4;
                    sprite.x += 8;
                } while (counter >= 0);
            }

            if ((CdaStatus.status & (CdlStatSeek | CdlStatRead)) == 0)
            {
                fade_step = 8;
            }
            scroll += StageScrollAdj[PSTATE->language][PSTATE->StageNo];
            xbase = scroll / 256;
            break;
        }

        DrawBG(background);
        fade += fade_step;
        if (fade < 0)
            fade = 0;
        else if (fade > 0xff)
            fade = 0xff;
        intensity = fade & 0xff;
        if (fade != 0)
        {
            draw_shade_quad_(OTablePt->org, intensity, intensity, intensity);
        }
        SkipFrame = SKIPFRAME_AFTER_LOAD;
        EndDrawing(0);
    }

    DisposeBG(background);
}
