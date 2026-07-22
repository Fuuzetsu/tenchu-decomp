#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/*
 * Save or restore the memory-card menu's VRAM window and allocate/free its
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

extern u_long *D_80097D20;
extern u8 *D_80097D24;
extern Sprite3D *D_80097D28;
extern Sprite3D *D_800C2D58[];

extern char D_80013C94[];
extern char D_80013CBC[];
extern char D_80013CE4[];
extern char D_80013D0C[];

extern void *valloc(u32 size);
extern void vfree(void *p);
extern s32 vsize(void *p);
extern Sprite3D *SetupSprite(Sprite3D *original, GsIMAGE *image);
extern s32 FUN_8005b17c(s32 page, s32 pad);

s32 FUN_8005adbc(s16 mode)
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
        LoadImage(&rect, D_80097D20);
        DrawSync(0);
        vfree(D_80097D20);
        D_80097D20 = 0;
        vfree(D_80097D24);
        vfree(D_80097D28);
        vfree(D_800C2D58[0]);
        vfree(D_800C2D58[1]);
        vfree(D_800C2D58[2]);
        vfree(D_800C2D58[3]);
        vfree(D_800C2D58[4]);
        return 0;
    }

    if (D_80097D20 == 0)
    {
        D_80097D20 = valloc(0x8000);
        rect.x = 0x3c0;
        rect.y = 0x100;
        rect.w = 0x40;
        rect.h = 0x100;
        StoreImage(&rect, D_80097D20);
        DrawSync(0);

        D_80097D24 = (u8 *)FileRead(D_80013C94);
        FUN_8005b17c(0, 0);
        i = 0;
        size = vsize(D_80097D24);
        if (size > 0)
        {
            do
            {
                c = D_80097D24[i];
                if ((c & 0x80) != 0)
                {
                    i++;
                }
                else if (c < 0x20 || c == 0x5c)
                {
                    D_80097D24[i] = 0;
                }
                i++;
            } while (i < size);
        }

        tim = FileRead(D_80013CBC);
        GetTIMInfo(tim, &image);
        LoadTIMAndFree(tim);
        D_80097D28 = SetupSprite(0, &image);
        D_80097D28->sprite.y = -0x3c;

        tim = FileRead(D_80013CE4);
        GetTIMInfo(tim, &image);
        LoadTIMAndFree(tim);
        D_800C2D58[0] = SetupSprite(0, &image);
        D_800C2D58[0]->sprite.h >>= 1;
        D_800C2D58[0]->sprite.my = image.ph >> 2;
        D_800C2D58[0]->sprite.y = 0x3c;

        D_800C2D58[1] = SetupSprite(D_800C2D58[0], 0);
        D_800C2D58[1]->sprite.v += image.ph >> 1;

        D_800C2D58[2] = SetupSprite(D_800C2D58[0], 0);
        D_800C2D58[2]->sprite.w = (D_800C2D58[0]->sprite.w >> 1) - 0x14;
        D_800C2D58[2]->sprite.mx = (D_800C2D58[2]->sprite.w >> 1) + 10;
        D_800C2D58[2]->sprite.x = -0x14;
        D_800C2D58[2]->sprite.u += 0xf;
        D_800C2D58[2]->sprite.attribute |= 0x30000000;

        D_800C2D58[3] = SetupSprite(D_800C2D58[0], 0);
        D_800C2D58[3]->sprite.w = (D_800C2D58[0]->sprite.w >> 1) - 10;
        D_800C2D58[3]->sprite.mx = D_800C2D58[3]->sprite.w >> 1;
        D_800C2D58[3]->sprite.x = 0x1c;
        D_800C2D58[3]->sprite.u += ((image.pw >> 1) * 4) + 10;
        D_800C2D58[3]->sprite.attribute |= 0x30000000;

        tim = FileRead(D_80013D0C);
        GetTIMInfo(tim, &image);
        LoadTIMAndFree(tim);
        D_800C2D58[4] = SetupSprite(0, &image);
        D_800C2D58[4]->sprite.x = 2;
        D_800C2D58[4]->sprite.y = 0x5a;
        return 1;
    }
    return 0;
}
