#include "common.h"
#include <psxsdk/libgpu.h>
#include "../main.exe/tuning.h"
#include "trial.exe.h"
#include "../main.exe/infoview.h"
#include "../main.exe/images.h"
#include "../main.exe/item.h"
#include "../main.exe/padcmd.h"
#include "../main.exe/sound.h"

/* TRIAL keeps the shared item/life/threat HUD, with a smaller pause loop
 * and no campaign map or debug-layout menus. */

extern s16 ForbiddenCommand[];


int ReqLifeBar(Humanoid *h)
{
    int i;
    int g;

    g = -1;
    for (i = 0; i < nLifeBar; i++)
    {
        if (LifeBar[i].count < 1)
        {
            if (g == -1)
            {
                g = i;
            }
        }
        else if (LifeBar[i].target == h)
        {
            g = i;
            break;
        }
    }
    if (g != -1)
    {
        LifeBar[g].target = h;
        LifeBar[g].style = LIFE_BAR_STYLE_ENEMY;
        LifeBar[g].life = h->life;
        LifeBar[g].max = h->lifemax;
        if (h->life == 0)
        {
            LifeBar[g].count = 100;
        }
        else
        {
            LifeBar[g].count = 300;
        }
        if (LifeBar[g].max < 1)
        {
            LifeBar[g].max = 1;
        }
        return 1;
    }
    return 0;
}

typedef struct
{
    s32 rotation;    /* +0x0, forwarded into both sprites verbatim */
    u8 frame_image;  /* +0x4 */
    u8 fill_image;   /* +0x5 */
} LifeBarSpriteEntry;

extern LifeBarSpriteEntry LifeBarParts[];

static void init_lifebar_(void)
{
    u8 image[25];
    GsSPRITE *slot;
    s32 tmp;
    int i;

    for (i = 0; i < N_LIFE_BAR_STYLES; i++)
    {
        slot = &LifeBarStyle[i].frame;
        InitSprite(GetImage(LifeBarParts[i].frame_image), slot);
        slot->mx = 0;
        slot->my = 0;
        slot->rotate = LifeBarParts[i].rotation;
        slot->attribute = GS_ATTR_SEMITRANS_AVERAGE;

        slot = &LifeBarStyle[i].fill;
        InitSprite(GetImage(LifeBarParts[i].fill_image), slot);
        slot->mx = 0;
        slot->my = 0;
        tmp = LifeBarParts[i].rotation;
        slot->attribute = GS_ATTR_SEMITRANS_ADD;
        slot->rotate = tmp;
    }
}


extern u8 fInitialize;

void InitializeInfoView(void)
{
    GsIMAGE *image;
    Sprite3D *item;
    int i;
    s32 padding_scale;
    s32 padding_attribute;

    image = GetImage(IMG_CURSOR);
    InitSprite(image, &CursorImage);
    CursorImage.attribute = GS_ATTR_SEMITRANS_ADD;
    image = GetImage(IMG_FONT_NUMBER);
    InitSprite(image, &NumberImage);
    for (i = 0; i < N_LOADOUT_ITEMS; i++)
    {
        image = GetImage(i + IMG_ICON_KAGINAWA);
        item = (ItemImage[i] = SetupSprite(0, image));
        item->scale = 0x3000;
        ItemImage[i]->attribute = MODEL_ATTR_CULL_BEHIND |
                                  MODEL_ATTR_CULL_SCREEN |
                                  MODEL_ATTR_CULL_FAR;
    }
    if (i < N_ITEM_SLOTS)
    {
        padding_scale = 0x3000;
        padding_attribute = MODEL_ATTR_CULL_BEHIND | MODEL_ATTR_CULL_SCREEN |
                            MODEL_ATTR_CULL_FAR;
        do
        {
            image = GetImage(IMG_GUNFIRE);
            item = (ItemImage[i] = SetupSprite(0, image));
            item->scale = padding_scale;
            ItemImage[i]->attribute = padding_attribute;
            i++;
        } while (i < N_ITEM_SLOTS);
    }
    for (i = 0; i < N_KEHAI_IMAGES; i++)
    {
        image = GetImage(i + IMG_KEHAI_GREEN);
        InitSprite(image, &KehaiImage[i]);
        KehaiImage[i].attribute = GS_ATTR_SEMITRANS_ADD;
    }
    ResetInfoview(-1);
    init_lifebar_();
    fInitialize = 1;
}


static void PutStrain(s32 x, s32 y)
{
    enum
    {
        STRAIN_WARNING_PERIOD = 30,
        STRAIN_BRIGHTNESS_RANGE = 255,
        STRAIN_DISPLAY_DISTANCE = 20000,
        STRAIN_DISTANCE_UNIT = 200
    };
    s32 ratio;
    GsSPRITE *spr;
    s32 delta;
    s32 s;
    /* Saved number-image U, then the pulse phase once the digits are drawn. */
    u16 value;

    ratio = StrainRatio;
    if (ratio == STRAIN_NO_THREAT)
    {
        return;
    }

    if (ratio == 0)
    {
        spr = &KehaiRedImage;
    }
    else if (ratio < -STRAIN_DISPLAY_DISTANCE)
    {
        spr = &KehaiCriticalImage;
        ratio = 0;
    }
    else if (ratio < 0)
    {
        spr = &KehaiYellowImage;
        ratio = 0;
        if (GameClock % STRAIN_WARNING_PERIOD == 0)
        {
            SoundEx(0, SE_WARNING_BEEP);
        }
    }
    else
    {
        GsSPRITE *img;
        s32 newpow;
        s32 r;

        if (ratio > STRAIN_DISPLAY_DISTANCE)
            return;
        spr = &KehaiGreenImage;
        NumberImage.w = 4;
        img = &NumberImage;
        img->x = (s16)(x + 0x22);
        value = img->u;
        img->y = (s16)(y + 8);
        newpow = (STRAIN_DISPLAY_DISTANCE - ratio) / STRAIN_DISTANCE_UNIT;
    strainloop:
        r = newpow / 10;
        img->u = value + (newpow % 10) * 4;
        GsSortSprite(img, OTablePt, 0);
        img->x -= 6;
        newpow = r;
        if (newpow != 0)
            goto strainloop;
        img->u = value;
    }

    delta = STRAIN_DISPLAY_DISTANCE - ratio;
    s = delta;
    if (delta < 0)
        s = delta + 0x1f;

    spr->x = (s16)x;
    spr->y = (s16)y;
    value = StrainPhase + (s >> 5);
    StrainPhase = value;
    spr->r = spr->g = spr->b =
        rsin(value) * 0x60 / FIXED_ONE + STRAIN_BRIGHTNESS_RANGE / 2;
    spr->scaley = spr->scalex =
        (s16)((delta << 0xb) / STRAIN_DISPLAY_DISTANCE) + FIXED_HALF;
    GsSortSprite(spr, OTablePt, 0);
}


static inline void SetLifeBarColour(GsSPRITE *sprite, s32 life, s32 maximum)
{
    enum
    {
        CRITICAL_DIVISOR = 4,
        NORMAL_BRIGHTNESS = 0x80,
        FLASH_BRIGHTNESS = 0xe6
    };

    if (maximum / CRITICAL_DIVISOR < life)
    {
        sprite->b = NORMAL_BRIGHTNESS;
        sprite->g = NORMAL_BRIGHTNESS;
        sprite->r = NORMAL_BRIGHTNESS;
        return;
    }
    if (GameClock & 1)
    {
        sprite->b = FLASH_BRIGHTNESS;
        sprite->g = FLASH_BRIGHTNESS;
        sprite->r = FLASH_BRIGHTNESS;
        return;
    }
    sprite->b = NORMAL_BRIGHTNESS;
    sprite->g = NORMAL_BRIGHTNESS;
    sprite->r = NORMAL_BRIGHTNESS;
}

static void PutLifeBar(s32 x, s32 y, s32 n, s32 mx, life_bar_style style)
{
    enum life_bar_rendering
    {
        LIFE_BAR_DIGIT_WIDTH = 4,
        LIFE_BAR_DIGIT_GAP = 6
    };
    GsSPRITE *digit_sprite;
    GsSPRITE *bar_sprite;
    s32 fill_height;
    s16 saved_height;
    s32 number_offset_x;
    s32 number_offset_y;
    s16 value; /* Saved digit U, then the signed fill scale. */

    {
        s32 px;
        s32 py;
        s32 count;

        count = n;
        number_offset_x = LifeBarStyle[style].dx;
        number_offset_y = LifeBarStyle[style].dy;
        NumberImage.w = LIFE_BAR_DIGIT_WIDTH;
        digit_sprite = &NumberImage;
        value = digit_sprite->u;
        px = x + number_offset_x;
        py = y + number_offset_y;
        digit_sprite->x = px;
        digit_sprite->y = py;

        {
            s32 next_count;

        loop:
            next_count = count / 10;
            digit_sprite->u = value + (count % 10) * LIFE_BAR_DIGIT_WIDTH;
            GsSortSprite(digit_sprite, OTablePt, 0);
            digit_sprite->x -= LIFE_BAR_DIGIT_GAP;
            count = next_count;
        }
        if (count != 0)
            goto loop;
    }
    digit_sprite->u = value;

    bar_sprite = &LifeBarStyle[style].frame;
    bar_sprite->x = x;
    bar_sprite->y = y;
    GsSortSprite(bar_sprite, OTablePt, 1);

    value = LifeBarStyle[style].scale;
    fill_height = value * n / mx;
    bar_sprite = &LifeBarStyle[style].fill;
    saved_height = bar_sprite->h;
    bar_sprite->h = LifeBarStyle[style].base + fill_height;

    SetLifeBarColour(bar_sprite, n, mx);

    bar_sprite->x = x;
    bar_sprite->y = y;
    GsSortSprite(bar_sprite, OTablePt, 0);
    bar_sprite->h = saved_height;
}


static s32 PutLifeBarS(void)
{
    s32 i;

    for (i = 0; i < nLifeBar; i++)
    {
        if (LifeBar[i].count > 0)
        {
            PutLifeBar(i * 60 - 140, -90, LifeBar[i].life,
                       LifeBar[i].max, LifeBar[i].style);
            LifeBar[i].count--;
        }
    }
    return 0;
}


static inline void DrawItemCursor(short x, short y, short size, s32 rotdif)
{
    CursorImage.x = x;
    CursorImage.y = y;
    CursorImage.scalex = size;
    CursorImage.scaley = size;
    CursorImage.rotate += rotdif;
    GsSortSprite(&CursorImage, OTablePt, 1);
}

static inline void PutNumberInline(int x, int y, int cols)
{
    int ou;

    ou = NumberImage.u;
    NumberImage.w = 4;
    NumberImage.x = (s16)x;
    NumberImage.y = (s16)y;
    do
    {
        NumberImage.u = ou + (cols % 10) * 4;
        GsSortSprite(&NumberImage, OTablePt, 0);
        NumberImage.x -= 6;
        cols /= 10;
    } while (cols != 0);
    NumberImage.u = ou;
}

static inline void DrawItemIcon(s32 ItemID, short x, short y, short scale)
{
    GsSPRITE *sprite = &ItemImage[ItemID]->sprite;

    sprite->x = x;
    sprite->y = y;
    sprite->scalex = scale;
    sprite->scaley = scale;
    GsSortSprite(sprite, OTablePt, 0);
}

static void PutItemList(void)
{
    enum
    {
        ItemX = 140,
        ItemY = 100,
        ItemGap = 20
    };
    s32 i;
    s32 x;

    SelectedItem = ITEM_NONE;
    x = ItemX;
    i = 0;
    while (1)
    {
        u32 s;

        if (i >= ITEM_N)
            break;

        s = CamState.Owner->item[i];
        if (s != 0)
        {
            s32 n;

            n = s;
            if (s != ITEM_INFINITE)
            {
                PutNumberInline(x + 22, ItemY, n);
            }

            if (ItemCursor == i)
            {
                /* Spin 6 deg/frame; GsSPRITE.rotate is degrees << 12. */
                DrawItemCursor(x, ItemY - 8, FIXED_ONE, -6 * FIXED_ONE);

                SelectedItem = i;
                DrawItemIcon(i, x, ItemY - 8, FIXED_ONE);
            }
            else
            {
                DrawItemIcon(i, x, ItemY - 8, FIXED_ONE * 2 / 3);
            }
            x -= ItemGap;
        }
        i++;
    }
}


static void DrawPause(int frame)
{
    DISPENV o_disp;
    DRAWENV o_draw;
    DRAWENV n_draw;
    POLY_GT4 ply;
    GsIMAGE *image;
    s32 phase;
    s32 bias;
    u8 far_col;

    GetDrawEnv(&o_draw);
    GetDispEnv(&o_disp);
    n_draw = o_draw;
    n_draw.clip = o_disp.disp;
    n_draw.ofs[0] = o_disp.disp.x;
    n_draw.ofs[1] = o_disp.disp.y;
    PutDrawEnv(&n_draw);
    image = GetImage(IMG_PAUSE);
    SetupImageToPolyGT4(image, &ply,
                       (s16)(SCREEN_W / 2 - image->pw * 2),
                       (s16)(SCREEN_H / 2 - (image->ph >> 1)));
    phase = (s16)frame * 0x44;
    bias = 0x80;
    far_col = rsin(phase) * 125 / FIXED_ONE + bias;
    phase += ANGLE_HALF_QUADRANT;
    ply.r0 = rsin(phase) * 125 / FIXED_ONE + bias;
    ply.g0 = ply.r0;
    ply.b0 = ply.r0;
    ply.r1 = ply.r0;
    ply.g1 = ply.r0;
    ply.b1 = ply.r0;
    ply.r2 = far_col;
    ply.g2 = far_col;
    ply.b2 = far_col;
    ply.r3 = far_col;
    ply.g3 = far_col;
    ply.b3 = far_col;
    DrawPrim(&ply);
    PutDrawEnv(&o_draw);
}

static void PauseProc(void)
{
    s16 initial_buttons;
    s16 buttons;
    s16 event_buttons;
    s16 pressed;
    s16 record_count;
    s16 frame;
    s16 next_count;
    s16 records[0x21];

    initial_buttons = GetPad(PAD_CONTROLLER_1);
    record_count = 0;
    frame = 0;
    if (((initial_buttons & PADstart) && !(SystemFlag & SYSFLAG_PAUSE)) ||
        get_pad_active_(PAD_CONTROLLER_1) == 0)
    {
        SystemFlag = SystemFlag | SYSFLAG_PAUSE;
        SoundEx((VECTOR *)0, SE_PAUSE_ENTER);
        VSync(0x14);
    }
    if (!(SystemFlag & SYSFLAG_PAUSE))
        return;
    buttons = initial_buttons;
    SkipFrame = SKIPFRAME_AFTER_LOAD;
    SsSetMVol(MASTER_VOLUME_MUTE, MASTER_VOLUME_MUTE);
    while (1)
    {
        event_buttons = buttons;
        PadProc();
        buttons = GetPad(PAD_CONTROLLER_1);
        pressed = buttons & (buttons ^ event_buttons);
        event_buttons = pressed;
        if (event_buttons & PADstart)
        {
            while (GetRealPad(PAD_PORT_1) & PADstart)
            {
                VSync(2);
            }
            SoundEx((VECTOR *)0, SE_MENU_CONFIRM);
            SystemFlag = SystemFlag & ~SYSFLAG_PAUSE;
            break;
        }
        if ((event_buttons & PADselect) && (SystemFlag & SYSFLAG_DEBUGMODE))
        {
            break;
        }
        if (event_buttons != 0)
        {
            if (record_count < 0x20)
            {
                records[record_count] = event_buttons;
                records[record_count + 1] = -1;
                next_count = record_count + 1;
                record_count = next_count;
                /* The shipped table has zero after its 20 buttons, but
                 * this comparison includes our -1 sentinel. Preserve that
                 * original mismatch rather than repairing the debug code. */
                if (memcmp(records, ForbiddenCommand,
                           (record_count + 1) * sizeof(s16)) == 0)
                {
                    SystemFlag |= SYSFLAG_DEBUGMODE;
                    SoundEx(0, SE_MENU_CONFIRM);
                }
            }
        }
        DrawPause(frame);
        VSync(2);
        frame++;
    }
    SsSetMVol(MASTER_VOLUME_MAX, MASTER_VOLUME_MAX);
}


void DoInfoViewProc(void)
{
    u16 pad;
    long trig;
    s32 i;

    pad = CamState.Owner->pad.data;
    trig = CamState.Owner->pad.trig;
    if (fInitialize == 0)
    {
        InitializeInfoView();
    }
    if ((pad & PADRup) == 0)
    {
        if ((trig & PADR2) != 0)
        {
            s32 c;

            i = ItemCursor;
            c = i;
            do
            {
                i--;
                if (i < 0)
                    i = ITEM_N;
            } while (CamState.Owner->item[i] == 0 && i != c);
            ItemCursor = i;
            SoundEx(0, SE_UI_CURSOR);
        }
        else if ((trig & PADL2) != 0)
        {
            s32 c;

            i = ItemCursor;
            c = i;
            do
            {
                i++;
                if (i > ITEM_N)
                    i = 0;
            } while (CamState.Owner->item[i] == 0 && i != c);
            ItemCursor = i;
            SoundEx(0, SE_UI_CURSOR);
        }
    }

    if (InfoViewPauseEnabled != 0 && GameClock > 10)
    {
        PauseProc();
    }
    PutItemList();
    PutLifeBar(-148, 105, CamState.Owner->life, CamState.Owner->lifemax,
               LIFE_BAR_STYLE_PLAYER);
    PutLifeBarS();
    PutStrain(-134, 92);
}


/* The shared signature survives, but TRIAL only clears the life bars. */
void ResetInfoview(int stage)
{
    int i;

    for (i = nLifeBar - 1; i >= 0; i--)
    {
        LifeBar[i].count = 0;
    }
}


void PutItemCursor(s16 x, s16 y, s16 size, s32 rotdif)
{
    DrawItemCursor(x, y, size, rotdif);
}


void PutItemIcon(s32 ItemID, short x, short y, short scale)
{
    DrawItemIcon(ItemID, x, y, scale);
}


void PutNumber(int x, int y, int cols)
{
    enum
    {
        NW = 4
    };
    enum
    {
        GAP = 6
    };
    int base;
    GsSPRITE *img;
    int q;

    NumberImage.w = NW;
    img = &NumberImage;
    base = img->u;
    img->x = (s16)x;
    img->y = (s16)y;
loop:
    q = cols / 10;
    img->u = base + (cols % 10) * NW;
    GsSortSprite(img, OTablePt, 0);
    img->x -= GAP;
    cols = q;
    if (cols != 0)
        goto loop;
    img->u = base;
}
