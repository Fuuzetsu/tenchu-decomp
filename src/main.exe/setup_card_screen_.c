#include "common.h"
#include "main.exe.h"
#include "images.h"
#include "memcard.h"
#include "vmemory.h"
#include <psxsdk/libgpu.h>

/* The card screen borrows a block of VRAM for its own textures and puts the
 * original contents back on the way out; McardVramSave is exactly that
 * block's 16-bit pixels, which is where its size comes from. */
#define MCARD_VRAM_X 0x3c0
#define MCARD_VRAM_Y 0x100
#define MCARD_VRAM_W 0x40
#define MCARD_VRAM_H 0x100

extern u_long *McardVramSave;
extern u8 *McardHelp;
extern Sprite3D *McardSprite;

extern char path_demo_start_card_j[];       /* K:\\WORK\\CDIMAGE\\DEMO\\start\\card_j.txt */
extern char path_demo_start_mcard_tim[];    /* K:\\WORK\\CDIMAGE\\DEMO\\start\\mcard.tim */
extern char path_demo_start_mbuttonj_tim[]; /* K:\\WORK\\CDIMAGE\\DEMO\\start\\mbuttonj.tim */
extern char path_demo_start_xtoselj_tim[];  /* K:\\WORK\\CDIMAGE\\DEMO\\start\\xtoselj.tim */

extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);

s32 setup_card_screen_(s16 operation)
{
    u8 c;
    s32 size;
    u_long *tim;
    s32 i;
    RECT rect;
    GsIMAGE image;

    if (operation != CARD_SCREEN_RESOURCES_ACQUIRE)
    {
        setRECT(&rect, MCARD_VRAM_X, MCARD_VRAM_Y, MCARD_VRAM_W, MCARD_VRAM_H);
        LoadImage(&rect, McardVramSave);
        DrawSync(0);
        vfree(McardVramSave);
        McardVramSave = 0;
        vfree(McardHelp);
        vfree(McardSprite);
        vfree(McardButtons[MCARD_BUTTON_CONFIRM_CANCEL]);
        vfree(McardButtons[MCARD_BUTTON_ACKNOWLEDGE]);
        vfree(McardButtons[MCARD_BUTTON_FORMAT_ACCEPT]);
        vfree(McardButtons[MCARD_BUTTON_FORMAT_CANCEL]);
        vfree(McardButtons[MCARD_BUTTON_FORMAT_SELECTION_HINT]);
        return 0;
    }

    if (McardVramSave == 0)
    {
        McardVramSave = valloc(MCARD_VRAM_W * MCARD_VRAM_H * 2);
        setRECT(&rect, MCARD_VRAM_X, MCARD_VRAM_Y, MCARD_VRAM_W, MCARD_VRAM_H);
        StoreImage(&rect, McardVramSave);
        DrawSync(0);

        McardHelp = (u8 *)FileRead(path_demo_start_card_j);
        draw_card_help_(0, 0);
        i = 0;
        size = vsize(McardHelp);
        if (size > 0)
        {
            do
            {
                c = McardHelp[i];
                if ((c & 0x80) != 0)
                {
                    i++;
                }
                else if (c < 0x20 || c == 0x5c)
                {
                    McardHelp[i] = 0;
                }
                i++;
            } while (i < size);
        }

        tim = FileRead(path_demo_start_mcard_tim);
        GetTIMInfo(tim, &image);
        LoadTIMAndFree(tim);
        McardSprite = SetupSprite(0, &image);
        McardSprite->sprite.y = -60;

        tim = FileRead(path_demo_start_mbuttonj_tim);
        GetTIMInfo(tim, &image);
        LoadTIMAndFree(tim);
        McardButtons[MCARD_BUTTON_CONFIRM_CANCEL] = SetupSprite(0, &image);
        McardButtons[MCARD_BUTTON_CONFIRM_CANCEL]->sprite.h >>= 1;
        McardButtons[MCARD_BUTTON_CONFIRM_CANCEL]->sprite.my = image.ph >> 2;
        McardButtons[MCARD_BUTTON_CONFIRM_CANCEL]->sprite.y = 60;

        McardButtons[MCARD_BUTTON_ACKNOWLEDGE] =
            SetupSprite(McardButtons[MCARD_BUTTON_CONFIRM_CANCEL], 0);
        McardButtons[MCARD_BUTTON_ACKNOWLEDGE]->sprite.v += image.ph >> 1;

        McardButtons[MCARD_BUTTON_FORMAT_ACCEPT] =
            SetupSprite(McardButtons[MCARD_BUTTON_CONFIRM_CANCEL], 0);
        McardButtons[MCARD_BUTTON_FORMAT_ACCEPT]->sprite.w =
            (McardButtons[MCARD_BUTTON_CONFIRM_CANCEL]->sprite.w >> 1) - 0x14;
        McardButtons[MCARD_BUTTON_FORMAT_ACCEPT]->sprite.mx =
            (McardButtons[MCARD_BUTTON_FORMAT_ACCEPT]->sprite.w >> 1) + 10;
        McardButtons[MCARD_BUTTON_FORMAT_ACCEPT]->sprite.x = -0x14;
        McardButtons[MCARD_BUTTON_FORMAT_ACCEPT]->sprite.u += 0xf;
        McardButtons[MCARD_BUTTON_FORMAT_ACCEPT]->sprite.attribute |=
            GS_ATTR_BLEND_MODE(GPU_BLEND_ADD_QUARTER);

        McardButtons[MCARD_BUTTON_FORMAT_CANCEL] =
            SetupSprite(McardButtons[MCARD_BUTTON_CONFIRM_CANCEL], 0);
        McardButtons[MCARD_BUTTON_FORMAT_CANCEL]->sprite.w =
            (McardButtons[MCARD_BUTTON_CONFIRM_CANCEL]->sprite.w >> 1) - 10;
        McardButtons[MCARD_BUTTON_FORMAT_CANCEL]->sprite.mx =
            McardButtons[MCARD_BUTTON_FORMAT_CANCEL]->sprite.w >> 1;
        McardButtons[MCARD_BUTTON_FORMAT_CANCEL]->sprite.x = 0x1c;
        McardButtons[MCARD_BUTTON_FORMAT_CANCEL]->sprite.u +=
            ((image.pw >> 1) * 4) + 10;
        McardButtons[MCARD_BUTTON_FORMAT_CANCEL]->sprite.attribute |=
            GS_ATTR_BLEND_MODE(GPU_BLEND_ADD_QUARTER);

        tim = FileRead(path_demo_start_xtoselj_tim);
        GetTIMInfo(tim, &image);
        LoadTIMAndFree(tim);
        McardButtons[MCARD_BUTTON_FORMAT_SELECTION_HINT] =
            SetupSprite(0, &image);
        McardButtons[MCARD_BUTTON_FORMAT_SELECTION_HINT]->sprite.x = 2;
        McardButtons[MCARD_BUTTON_FORMAT_SELECTION_HINT]->sprite.y = 90;
        return 1;
    }
    return 0;
}
