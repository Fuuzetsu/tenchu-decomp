#include "common.h"
#include "main.exe.h"
#include "vmemory.h"
#include <psxsdk/libgpu.h>

/*
 * setup_card_screen_ (0x8005adbc) — Save or restore the memory-card menu's VRAM window and allocate/free its
 * help text and sprites.  The load path also sanitises the help text, skipping
 * the second byte of high-bit characters and replacing control/backslash
 * bytes with NUL separators.
 *
 * Matching notes:
 *  - `rect` followed by `image` gives the target's complete sp+0x10..0x37
 *    workspace and 0x50-byte frame.
 *  - `i = 0` before vsize keeps the loop index live across that call in s0;
 *    the do-while then reproduces both increments on high-bit characters.
 *  - Literal returns are intentional.  A shared `result` local allocated in
 *    a0, shortened the success tail, and added a final move.  Independent
 *    `return 0`/`return 1` sites keep the result in v0 and make cc1 move the
 *    final SetupSprite result to a0 before the two field stores.
 */

extern u_long *McardVramSave;
extern u8 *McardHelp;
extern Sprite3D *McardSprite;
extern Sprite3D *McardButtons[];

extern char path_demo_start_card_j[];       /* K:\\WORK\\CDIMAGE\\DEMO\\start\\card_j.txt */
extern char path_demo_start_mcard_tim[];    /* K:\\WORK\\CDIMAGE\\DEMO\\start\\mcard.tim */
extern char path_demo_start_mbuttonj_tim[]; /* K:\\WORK\\CDIMAGE\\DEMO\\start\\mbuttonj.tim */
extern char path_demo_start_xtoselj_tim[];  /* K:\\WORK\\CDIMAGE\\DEMO\\start\\xtoselj.tim */

extern void *valloc(u32 size);
extern void vfree(void *p);
extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);
extern s32 draw_card_help_(s32 page, s32 pad);

s32 setup_card_screen_(s16 mode)
{
    u8 c;
    s32 size;
    u_long *tim;
    s32 i;
    RECT rect;
    GsIMAGE image;

    if (mode != 0)
    {
        rect.x = 0x3c0;
        rect.y = 0x100;
        rect.w = 0x40;
        rect.h = 0x100;
        LoadImage(&rect, McardVramSave);
        DrawSync(0);
        vfree(McardVramSave);
        McardVramSave = 0;
        vfree(McardHelp);
        vfree(McardSprite);
        vfree(McardButtons[0]);
        vfree(McardButtons[1]);
        vfree(McardButtons[2]);
        vfree(McardButtons[3]);
        vfree(McardButtons[4]);
        return 0;
    }

    if (McardVramSave == 0)
    {
        McardVramSave = valloc(0x8000);
        rect.x = 0x3c0;
        rect.y = 0x100;
        rect.w = 0x40;
        rect.h = 0x100;
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
        McardButtons[0] = SetupSprite(0, &image);
        McardButtons[0]->sprite.h >>= 1;
        McardButtons[0]->sprite.my = image.ph >> 2;
        McardButtons[0]->sprite.y = 60;

        McardButtons[1] = SetupSprite(McardButtons[0], 0);
        McardButtons[1]->sprite.v += image.ph >> 1;

        McardButtons[2] = SetupSprite(McardButtons[0], 0);
        McardButtons[2]->sprite.w = (McardButtons[0]->sprite.w >> 1) - 0x14;
        McardButtons[2]->sprite.mx = (McardButtons[2]->sprite.w >> 1) + 10;
        McardButtons[2]->sprite.x = -0x14;
        McardButtons[2]->sprite.u += 0xf;
        McardButtons[2]->sprite.attribute |= 0x30000000;

        McardButtons[3] = SetupSprite(McardButtons[0], 0);
        McardButtons[3]->sprite.w = (McardButtons[0]->sprite.w >> 1) - 10;
        McardButtons[3]->sprite.mx = McardButtons[3]->sprite.w >> 1;
        McardButtons[3]->sprite.x = 0x1c;
        McardButtons[3]->sprite.u += ((image.pw >> 1) * 4) + 10;
        McardButtons[3]->sprite.attribute |= 0x30000000;

        tim = FileRead(path_demo_start_xtoselj_tim);
        GetTIMInfo(tim, &image);
        LoadTIMAndFree(tim);
        McardButtons[4] = SetupSprite(0, &image);
        McardButtons[4]->sprite.x = 2;
        McardButtons[4]->sprite.y = 90;
        return 1;
    }
    return 0;
}
