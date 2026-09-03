#include "common.h"
#include <psxsdk/libgpu.h>
#include "tuning.h"
#include "main.exe.h"
#include "model.h"
#include "tim.h"
#include "adt.h"
#include "effect.h"
#include "infoview.h"
#include "images.h"
#include "item.h"
#include "layout_save.h"
#include "misc.h"
#include "padcmd.h"
#include "sound.h"

/*
 * Retail reorganises INFOVIEW.C and splits its debug tools into a later block.
 * The translation-unit manifest retains the earlier debug-symbol order.
 */

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int ReqLifeBar(struct Humanoid *h);
 *     INFOVIEW.C:89, 26 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * h
 *     reg   $a1       int i
 *     reg   $a2       int g
 *
 * Globals it touches, as the original declared them:
 *     extern struct INFOVIEW__198fake LifeBar[4];
 * END PSX.SYM */

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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitializeInfoView(void);
 *     INFOVIEW.C:119, 74 src lines, frame 64 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       int i
 *     stack sp+16     unsigned char [25] image
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE CursorImage;
 *     extern struct Sprite3D *ItemImage[25];
 *     extern unsigned char fInitialize;
 * END PSX.SYM */

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
    leResetEnemyLayout();
    ResetInfoview(-1);
    init_lifebar_();
    fInitialize = 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutStrain(void);
 *     INFOVIEW.C:218, 70 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       int newpow
 *     reg   $a2       int r
 *     reg   $s1       struct GsSPRITE * spr
 *
 * Globals it touches, as the original declared them:
 *     extern long StrainRatio;
 *     extern long GameClock;
 *     extern struct GsSPRITE NumberImage;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

extern s32 StrainRatio;
extern u16 StrainPhase;

static void PutStrain(s32 x, s32 y)
{
    enum
    {
        speed = 30
    };
    enum
    {
        range = 255,
        powrange = 20000
    };
    s32 ratio;
    GsSPRITE *spr;
    s32 delta;
    s32 s;
    u16 phase;

    ratio = StrainRatio;
    if (ratio != 0x7fffffff)
    {
        if (ratio == 0)
        {
            spr = &KehaiRedImage;
        }
        else if (ratio < -powrange)
        {
            spr = &KehaiCriticalImage;
            ratio = 0;
        }
        else if (ratio < 0)
        {
            spr = &KehaiYellowImage;
            ratio = 0;
            if (GameClock % speed == 0)
            {
                SoundEx(0, SE_WARNING_BEEP);
            }
        }
        else
        {
            u8 base;
            GsSPRITE *img;
            s32 newpow;
            s32 r;

            if (ratio > powrange)
                return;
            spr = &KehaiGreenImage;
            NumberImage.w = 4;
            img = &NumberImage;
            img->x = (s16)(x + 0x22);
            base = img->u;
            img->y = (s16)(y + 8);
            newpow = (powrange - ratio) / 200;
        strainloop:
            r = newpow / 10;
            img->u = base + (newpow % 10) * 4;
            GsSortSprite(img, OTablePt, 0);
            img->x -= 6;
            newpow = r;
            if (newpow != 0)
                goto strainloop;
            do
            {
                img->u = base;
            } while (0);
        }

        delta = powrange - ratio;
        s = delta;
        if (delta < 0)
            s = delta + 0x1f;

        spr->x = (s16)x;
        spr->y = (s16)y;
        phase = StrainPhase + (s >> 5);
        StrainPhase = phase;
        spr->r = spr->g = spr->b =
            rsin(phase) * 0x60 / FIXED_ONE + range / 2;
        spr->scaley = spr->scalex =
            (s16)((delta << 0xb) / powrange) + FIXED_HALF;
        GsSortSprite(spr, OTablePt, 0);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutLifeBar(int x, int y, int n, int mx, int style);
 *     INFOVIEW.C:292, 32 src lines, frame 72 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       int x
 *     param $s4       int y
 *     param $s1       int n
 *     param $a3       int mx
 *     param stack+16  int style
 *     reg   $s5       int style
 *     stack sp+16     struct POLY_F4 poly
 *     reg   $a3       int w
 *     reg   $v0       int x
 *     reg   $a0       int y
 *     reg   $a3       int n
 *     reg   $s2       int ou
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE NumberImage;
 *     extern struct INFOVIEW__196fake LifeBarStyle[2];
 *     extern struct GsOT *OTablePt;
 *     extern long GameClock;
 * END PSX.SYM */

static void PutLifeBar(s32 x, s32 y, s32 n, s32 mx, life_bar_style style)
{
    GsSPRITE *img;
    GsSPRITE *ou;
    s32 q;
    s16 oldh;
    s32 color;
    s32 dx;
    s32 dy;
    s32 u;

    {
        s32 px;
        s32 py;
        s32 count;

        count = n;
        NumberImage.w = (dx = LifeBarStyle[style].dx,
                         dy = LifeBarStyle[style].dy, 4);
        img = &NumberImage;
        u = img->u;
        px = x + dx;
        py = y + dy;
        img->x = px;
        img->y = py;

        {
            s32 q;

        loop:
            q = count / 10;
            img->u = u + (count % 10) * 4;
            GsSortSprite(img, OTablePt, 0);
            img->x -= 6;
            count = q;
        }
        if (count != 0)
            goto loop;
    }
    img->u = u;

    ou = &LifeBarStyle[style].frame;
    ou->x = x;
    ou->y = y;
    GsSortSprite(ou, OTablePt, 1);

    q = LifeBarStyle[style].scale * n / mx;
    ou = &LifeBarStyle[style].fill;
    oldh = ou->h;
    ou->h = LifeBarStyle[style].base + q;

    if (mx / 4 < n)
        color = 0x80;
    else
    {
        color = GameClock & 1;
        if (color != 0)
            color = 0xE6;
        else
            color = 0x80;
    }
    ou->b = color;
    ou->g = color;
    if (u != 0)
    {
        ou->r = color;
    }
    else
    {
        ou->r = color;
    }

    ou->x = x;
    ou->y = y;
    GsSortSprite(ou, OTablePt, 0);
    ou->h = oldh;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int PutLifeBarS(void);
 *     INFOVIEW.C:328, 16 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s2       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct INFOVIEW__198fake LifeBar[4];
 * END PSX.SYM */

static s32 PutLifeBarS(void)
{
    s32 i;

    i = 0;
    do
    {
        if (LifeBar[i].count > 0)
        {
            PutLifeBar(i * 60 - 140, -90, LifeBar[i].life,
                       LifeBar[i].max, LifeBar[i].style);
            LifeBar[i].count--;
        }
        i++;
    } while (i < nLifeBar);
    return 0;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutItemList(void);
 *     INFOVIEW.C:366, 35 src lines, frame 56 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s3       int i
 *     reg   $s4       int x
 *     reg   $s0       unsigned int s
 *     reg   $v1       int n
 *     reg   $s2       int ou
 *     reg   $s3       int ItemID
 *     reg   $a0       struct GsSPRITE * spr
 *     reg   $s3       int ItemID
 *     reg   $a0       struct GsSPRITE * spr
 *
 * Globals it touches, as the original declared them:
 *     extern short SelectedItem;
 *     extern struct GsSPRITE NumberImage;
 *     extern struct GsSPRITE CursorImage;
 *     extern struct TCameraStatus CamState;
 *     extern struct GsOT *OTablePt;
 *     extern short ItemCursor;
 *     extern struct Sprite3D *ItemImage[25];
 * END PSX.SYM */

static inline void PutItemCursorInline(short x, short y, short size, s32 rotdif)
{
    CursorImage.x = x;
    CursorImage.y = y;
    CursorImage.scalex = size;
    CursorImage.scaley = size;
    CursorImage.rotate += rotdif;
    GsSortSprite(&CursorImage, OTablePt, 1);
}

static inline void PutNumberInline(int x, int y, int cols, int n)
{
    int ou;
    int q;

    ou = NumberImage.u;
    NumberImage.w = 4;
    NumberImage.x = (s16)x;
    NumberImage.y = (s16)y;
loop:
    q = cols / 10;
    NumberImage.u = ou + (cols % 10) * 4;
    GsSortSprite(&NumberImage, OTablePt, 0);
    NumberImage.x -= 6;
    cols = q;
    if (cols != 0)
        goto loop;
    NumberImage.u = ou;
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
                PutNumberInline(x + 22, ItemY, n, 0);
            }

            if (ItemCursor == i)
            {
                GsSPRITE *spr;

                PutItemCursorInline(x, ItemY - 8, FIXED_ONE, -6 * FIXED_ONE); /* spin 6 deg/frame (GsSPRITE.rotate is degrees<<12) */

                SelectedItem = i;
                spr = &ItemImage[i]->sprite;
                spr->x = x;
                spr->y = ItemY - 8;
                spr->scalex = FIXED_ONE;
                spr->scaley = FIXED_ONE;
                GsSortSprite(spr, OTablePt, 0);
            }
            else
            {
                GsSPRITE *spr;

                spr = &ItemImage[i]->sprite;
                spr->x = x;
                spr->y = ItemY - 8;
                spr->scalex = FIXED_ONE * 2 / 3;
                spr->scaley = FIXED_ONE * 2 / 3;
                GsSortSprite(spr, OTablePt, 0);
            }
            x -= ItemGap;
        }
        i++;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void AddItem2(void);
 *     INFOVIEW.C:958, 27 src lines, frame 240 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+24     struct PARAM_ITEM_STAY param
 *     reg   $s2       long x
 *     reg   $s0       long y
 *     reg   $s1       long z
 *     stack sp+24     struct TAdtSelect [25] ItemName
 *     stack sp+48     struct SVECTOR vec
 * END PSX.SYM */

extern char str_select_item[]; /* "select item" */


static void AddItem2(void)
{
    s32 n;
    s32 sx, cx;
    s32 x, y, z;
    s32 h;
    ModelArchiveType *pm;

    {
        {
            TAdtSelect ItemName[ITEM_N];

            __builtin_memcpy(ItemName, DEBUG_MENU_ITEM_CHOICE_OPTIONS,
                             sizeof(ItemName));
            n = AdtSelect(str_select_item, ItemName, 0);
        }
    }

    {
        PARAM_ITEM_STAY param = {
            .type = n
        };
        SVECTOR vec;

        sx = rsin(CamState.Owner->model->rotate.vy) * 1000;
        pm = CamState.Owner->model;
        if (sx < 0)
            sx += FIXED_TRUNC_BIAS;
        h = pm->locate.coord.t[1];
        y = h;
        x = pm->locate.coord.t[0] - (sx >> FIXED_SHIFT);
        cx = rcos(pm->rotate.vy) * 1000;
        pm = CamState.Owner->model;
        z = pm->locate.coord.t[2] - (cx / FIXED_ONE);
        h = GetAreaMapLevel(GlobalAreaMap, x, y, z, AREA_LEVEL_STEP_DOWN);
        if (h != LEVEL_NONE)
        {
            param.locate.vx = x;
            param.locate.vy = h;
            param.locate.vz = z;
            ReqItemStay(&param);
            vec = (SVECTOR){
                .vx = 0,
                .vy = -600,
                .vz = 0
            };
            SetSmoke(&param.locate, &vec, 3, 10);
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawPause(void);
 *     INFOVIEW.C:1224, 35 src lines, frame 304 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct DISPENV o_disp
 *     stack sp+40     struct DRAWENV o_draw
 *     stack sp+136    struct DRAWENV n_draw
 *     stack sp+232    struct POLY_GT4 ply
 *
 * Globals it touches, as the original declared them:
 *     extern enum TSystemFlag SystemFlag;
 * END PSX.SYM */

static void DrawPause(int frame)
{
    DISPENV o_disp;
    DRAWENV o_draw;
    DRAWENV n_draw;
    POLY_GT4 ply;
    GsIMAGE *image;
    s32 t;
    s32 bias;
    u8 far_col;

    if ((SystemFlag & SYSFLAG_DEBUGPRINT) == 0)
    {
        GetDrawEnv(&o_draw);
        GetDispEnv(&o_disp);
        n_draw = o_draw;
        n_draw.clip = o_disp.disp;
        n_draw.ofs[0] = o_disp.disp.x;
        n_draw.ofs[1] = o_disp.disp.y;
        PutDrawEnv(&n_draw);
        image = GetImage(IMG_PAUSE);
        SetupImageToPolyGT4(image, &ply, (s16)(0xA0 - image->pw * 2), (s16)(0x78 - (image->ph >> 1)));
        t = (s16)frame * 0x44;
        /* Empty loop retained for code layout; its original source construct is unknown. */
        do
        {
        } while (0);
        bias = 0x80;
        far_col = rsin(t) * 125 / FIXED_ONE + bias;
        ply.r0 = rsin(t + ANGLE_HALF_QUADRANT) * 125 / FIXED_ONE + bias;
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
}

extern char str_select_item[]; /* select item */
extern char str_number_of[];   /* number of */
/* Retail data: Left Right Left Right, Cross x2, Circle x2, Square x2,
 * Triangle x2 — the debug item-grant code. */
extern s16 CheatSeq[];
/* The retail command grew from the demo's original short [15] to 21
 * entries (20 buttons + the -1 terminator): Triangle Cross Square Circle, Cross x4, Triangle x4,
 * Square Circle Triangle Cross, Square x2, Circle x2 — the debug-mode
 * code. */
extern s16 ForbiddenCommand[21];


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern enum TSystemFlag SystemFlag;
 * END PSX.SYM */

void CheckCheatCodes(s16 *rec, int n)
{
    s32 sel;
    TAdtSelect menu_options[ITEM_N];

    if (memcmp(rec, CheatSeq, n << 1) == 0)
    {
        SoundEx(0, SE_MENU_CONFIRM);
        __builtin_memcpy(menu_options, DEBUG_MENU_ITEM_CHOICE_OPTIONS,
                         sizeof(DEBUG_MENU_ITEM_CHOICE_OPTIONS));
        sel = AdtSelect(str_select_item, menu_options, 0);
        __builtin_memcpy(menu_options, sel_quantity, sizeof(sel_quantity));
        CamState.Owner->item[sel] +=
            AdtSelect(str_number_of, menu_options, 0);
        SoundEx(0, SE_ITEM_USE);
    }
    else
    {
        if (memcmp(rec, ForbiddenCommand, n << 1) != 0)
        {
            return;
        }
        SystemFlag |= SYSFLAG_DEBUGMODE;
        SoundEx(0, SE_MENU_CONFIRM);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PauseProc(void);
 *     INFOVIEW.C:1261, 56 src lines, frame 56 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       short pad
 *     reg   $s2       short ForbiddenCount
 *     reg   $s1       short push
 *     reg   $v1       short trig
 *     stack sp+24     struct RECT rc
 *
 * Globals it touches, as the original declared them:
 *     extern enum TSystemFlag SystemFlag;
 *     extern short SkipFrame;
 *     extern struct TCameraStatus CamState;
 *     extern short ActionHalt;
 *     extern short Findenemies;
 * END PSX.SYM */

/* Retail declares s16(s32) here; get_pad_active_ defines u8(s16). */
extern short get_pad_active_(s32 arg);
extern short check_cheat_command_(short pad, short trg);

static void PauseProc(void)
{
    s16 pad;
    s16 cur;
    s16 opad;
    s16 trig;
    int com;
    s16 i;
    s16 cnt;
    s16 j;
    s16 buf[0x21];

    pad = GetPad(PAD_CONTROLLER_1);
    i = 0;
    cnt = 0;
    if (((pad & PADstart) && !(SystemFlag & SYSFLAG_PAUSE)) ||
        get_pad_active_(PAD_CONTROLLER_1) == 0)
    {
        SystemFlag = (SystemFlag | SYSFLAG_PAUSE) & ~SYSFLAG_DEBUG_SELECT;
        SoundEx((VECTOR *)0, SE_PAUSE_ENTER);
        VSync(0x14);
    }
    if (!(SystemFlag & SYSFLAG_PAUSE))
        return;
    cur = pad;
    SkipFrame = SKIPFRAME_AFTER_LOAD;
    SsSetMVol(MASTER_VOLUME_MUTE, MASTER_VOLUME_MUTE);
    while (1)
    {
        opad = cur;
        PadProc();
        cur = GetPad(PAD_CONTROLLER_1);
        trig = cur & (cur ^ opad);
        opad = trig;
        if (cur == (PADstart | PADselect))
            return_to_menu_();
        com = check_cheat_command_(cur, trig);
        /* Motion ids above the taunt (0x713) are the stealth-kill
         * finishers — no cheating mid-finisher. */
        if (CamState.Owner->status == STAT_ATTACK && CamState.Owner->motion->mid > MOT_ATTACK_TAUNT)
            com = CHEAT_NONE;
        if (com == CHEAT_REVIVE)
        {
            if (CamState.Owner->status != STAT_DEAD)
            {
                CamState.Owner->life = CamState.Owner->lifemax;
                dispose_weapon_data_of_char_(CamState.Owner,
                                             ATTACK_CANCEL_ALL);
                CamState.Owner->status = STAT_NORMAL;
                ActionHalt = ACTION_HALT_NONE;
                Sound(CamState.Owner, SE_ITEM_USE);
                SetCameraMode(CMODE_NORMAL);
                Findenemies++;
                SystemFlag = SystemFlag & ~SYSFLAG_PAUSE;
                CamState.Owner->pad.data = PADRleft;
                break;
            }
            continue;
        }
        if (com == CHEAT_DEBUG_MENU)
        {
            SystemFlag = SystemFlag | SYSFLAG_DEBUGMODE;
            SoundEx((VECTOR *)0, SE_MENU_CONFIRM);
            break;
        }
        if (opad & PADstart)
        {
            while (1)
            {
                if (!(GetRealPad(PAD_PORT_1) & PADstart))
                    break;
                VSync(2);
            }
            SoundEx((VECTOR *)0, SE_MENU_CONFIRM);
            SystemFlag = SystemFlag & ~SYSFLAG_PAUSE;
            break;
        }
        if ((opad & PADselect) && (SystemFlag & SYSFLAG_DEBUGMODE))
        {
            SystemFlag = SystemFlag | SYSFLAG_DEBUG_SELECT;
            break;
        }
        if (opad != 0)
        {
            if (i < 0x20)
            {
                buf[i] = opad;
                buf[i + 1] = -1;
                j = i + 1;
                i = j;
                CheckCheatCodes(buf, j + 1);
            }
        }
        if ((SystemFlag & (SYSFLAG_DEBUGMODE | SYSFLAG_DEBUG_SELECT)) !=
                (SYSFLAG_DEBUGMODE | SYSFLAG_DEBUG_SELECT) ||
            (pad & PADstart))
            DrawPause(cnt);
        VSync(2);
        cnt++;
    }
    SsSetMVol(MASTER_VOLUME_MAX, MASTER_VOLUME_MAX);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutMap(void);
 *     INFOVIEW.C:1322, 65 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s2       int rgb
 *     reg   $s1       struct POLY_XF4 * ply
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char PutMapMode;
 *     extern struct GsSPRITE MapImage;
 *     extern struct GsOT *OTablePt;
 *     extern int StageID;
 * END PSX.SYM */

extern s32 MapSlideX;
extern s32 MapSlideY;
extern MapPlacementType MapPlacement[N_STAGE_CONFIGS];


static void PutMap(void)
{
    POLY_XF4 *ply;
    s32 rgb;

    ply = (POLY_XF4 *)GsGetWorkBase();
    GsSetWorkBase(ply + 1);
    SetPolyXF4(ply, GPU_BLEND_SUBTRACT);
    setXY4(&ply->ply, -SCREEN_W / 2, -SCREEN_H / 2, SCREEN_W / 2,
           -SCREEN_H / 2, -SCREEN_W / 2, SCREEN_H / 2, SCREEN_W / 2,
           SCREEN_H / 2);
    rgb = (0xA0 - MapSlideX) / 4;

    switch (PutMapMode)
    {
    case PUTMAP_OPEN:
        MapSlideX = 160;
        MapSlideY = 0;
        PutMapMode = PUTMAP_SLIDE_IN;
        ply->ply.r0 = 0;
        ply->ply.g0 = 0;
        ply->ply.b0 = 0;
        SoundEx((VECTOR *)0, SE_MAP_OPEN);
        break;
    case PUTMAP_SLIDE_IN:
        MapImage.r = 0x3C;
        MapImage.g = 0x3C;
        MapImage.b = 0x3C;
        MapImage.scalex = FIXED_ONE;
        MapImage.scaley = FIXED_ONE;
        MapImage.x = MapSlideX;
        MapImage.y = MapSlideY;
        GsSortSprite(&MapImage, OTablePt, 2);
        MapImage.r = 0x80;
        MapImage.g = 0x80;
        MapImage.b = 0x80;
        ply->ply.r0 = rgb;
        ply->ply.g0 = rgb;
        ply->ply.b0 = rgb;
        MapSlideX -= 0x28;
        if (MapSlideX <= 0)
        {
            PutMapMode++;
        }
        break;
    case PUTMAP_SHOWN:
        MapSlideX = 0;
        MapSlideY = 0;
        ply->ply.r0 = rgb;
        ply->ply.g0 = rgb;
        ply->ply.b0 = rgb;
        draw_map_items_(CamState.Owner->model->locate.coord.t[0],
                        CamState.Owner->model->locate.coord.t[2],
                        &MapPlacement[StageID]);
        break;
    }

    MapImage.scalex = FIXED_ONE;
    MapImage.scaley = FIXED_ONE;
    MapImage.x = MapSlideX;
    MapImage.y = MapSlideY;
    GsSortSprite(&MapImage, OTablePt, 1);
    AddXF4(OTablePt->org + 2, ply);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DoInfoViewProc(void);
 *     INFOVIEW.C:1413, 132 src lines, frame 344 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       long trig
 *     stack sp+32     struct TAdtSelect [12] Option
 *     reg   $v1       int i
 *     stack sp+128    struct TAdtSelect [4] Num
 *     stack sp+128    struct TAdtSelect [25] ItemName
 *     reg   $s0       int i
 *     stack sp+168    struct TAdtSelect [3] OkCancel
 *     stack sp+128    struct TAdtSelect [5] Option
 *     reg   $v1       int i
 *     stack sp+128    struct TAdtSelect [5] option
 *     reg   $v1       int c
 *     reg   $v1       int c
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern unsigned char fInitialize;
 *     extern enum TSystemFlag SystemFlag;
 *     extern short ItemCursor;
 *     extern long GameClock;
 *     extern unsigned char PutMapMode;
 * END PSX.SYM */

/* gp-relative — defined by this (info-view) TU; Build.hs maspsxGpExterns */
extern u8 fInitialize;

extern char str_select_item[]; /* "select item" */
extern char str_number_of[]; /* "number of" */
extern char str_item_layout_option[]; /* "item layout option" */
extern char msg_clear_ok[]; /* "clear ok?" */
extern char str_select_option[]; /* "select option" */
extern char str_opt[]; /* "opt" — the effect-menu title */


static inline void ItemAddMenu(void)
{
    s32 n;
    TAdtSelect menu_options[ITEM_N];

    __builtin_memcpy(menu_options, DEBUG_MENU_ITEM_CHOICE_OPTIONS,
                     sizeof(DEBUG_MENU_ITEM_CHOICE_OPTIONS));
    n = AdtSelect(str_select_item, menu_options, 0);
    __builtin_memcpy(menu_options, sel_quantity, sizeof(sel_quantity));
    CamState.Owner->item[n] += AdtSelect(str_number_of, menu_options, 0);
}

static inline void ItemLayoutMenu(void)
{
    enum
    {
        ITEM_LAYOUT_SET = 0,
        ITEM_LAYOUT_CLEAR_ALL = 1
    };
    TAdtSelect Option[5];
    TAdtSelect OkCancel[3];

    __builtin_memcpy(Option, DEBUG_MENU_ITEM_LAYOUT_OPTIONS, sizeof(Option));
    __builtin_memcpy(OkCancel, sel_okcancel2, sizeof(OkCancel));
    switch (AdtSelect(str_item_layout_option, Option, 0))
    {
    case ITEM_LAYOUT_SET:
        AddItem2();
        break;
    case ITEM_LAYOUT_CLEAR_ALL:
        if (AdtSelect(msg_clear_ok, OkCancel, 1) == 1)
        {
            ClearItemLayout();
        }
        break;
    }
}

static inline void EffectSpawnMenu(void)
{
    TAdtSelect Option[31];

    __builtin_memcpy(Option, DEBUG_MENU_HIDDEN_EFFECT_SPAWN_OPTIONS,
                     sizeof(Option));
    AddMisc(MISC_DOOR, CamState.Owner->model->locate.coord.t[0],
            CamState.Owner->model->locate.coord.t[1],
            CamState.Owner->model->locate.coord.t[2],
            CamState.Owner->model->rotate.vy,
            AdtSelect(str_opt, Option, 0), 0);
}

void DoInfoViewProc(void)
{
    enum
    {
        ENEMY = 0,
        ITEM = 1,
        CHARGE = 2,
        FILE = 3,
        PLAYER = 4,
        STAGE = 5,
        HIDDEN_EFFECT = 0x63
    };
    u16 pad;
    long trig;
    s32 i;
    TAdtSelect Option[11];

    pad = CamState.Owner->pad.data;
    trig = CamState.Owner->pad.trig;
    if (fInitialize == 0)
    {
        InitializeInfoView();
    }
    if ((SystemFlag & SYSFLAG_DEBUGMODE) && (u16)GetPad(PAD_CONTROLLER_1) == (PADL2 | PADR2))
    {
        __builtin_memcpy(Option, DEBUG_MENU_MAIN_SCREEN_OPTIONS,
                         sizeof(Option));
        VISIBLE_ENEMIES_ = 0;
        switch (AdtSelect(str_select_option, Option, 0))
        {
        case ENEMY:
            LayoutEnemyOption();
            break;
        case CHARGE:
            ItemAddMenu();
            break;
        case ITEM:
            ItemLayoutMenu();
            break;
        case FILE:
            FileOption();
            break;
        case PLAYER:
            PlayerOption();
            break;
        case STAGE:
            debug_menu_stage_option();
            break;
        case HIDDEN_EFFECT:
            EffectSpawnMenu();
            break;
        }
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

    if (GameClock > 10)
    {
        PauseProc();
    }
    PutItemList();
    PutLifeBar(-148, 105, CamState.Owner->life, CamState.Owner->lifemax,
               LIFE_BAR_STYLE_PLAYER);
    PutLifeBarS();
    PutStrain(-134, 92);
    if ((GetPad(PAD_CONTROLLER_1) & PADselect) &&
        (SystemFlag & (SYSFLAG_DEBUGPRINT | SYSFLAG_PAUSE)) == 0)
    {
        PutMap();
    }
    else
    {
        PutMapMode = PUTMAP_OPEN;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ResetInfoview(int stage);
 *     INFOVIEW.C:1391, 18 src lines, frame 56 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int stage
 *     stack sp+16     struct GsIMAGE image
 *
 * Globals it touches, as the original declared them:
 *     extern struct INFOVIEW__198fake LifeBar[4];
 *     extern unsigned char *ImagePath;
 * END PSX.SYM */

extern char path_chizu_tim[]; /* chizu.tim */

void ResetInfoview(int stage)
{
    int i;
    u_long *adr;
    GsIMAGE image;

    for (i = nLifeBar - 1; i >= 0; i--)
    {
        LifeBar[i].count = 0;
    }
    if (stage >= 0)
    {
        adr = PathFileRead(ImagePath, path_chizu_tim);
        GetTIMInfo(adr, &image);
        LoadTIMAndFree(adr);
        InitSprite(&image, &MapImage);
    }
}


void return_to_menu_proc_(void)
{
    return_to_menu_();
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DemoPatchInit(void);
 *     INFOVIEW.C:1196, 8 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct RECT rc
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char DemoBackupArea[64];
 * END PSX.SYM */

extern u8 DemoBackupArea[64];

void DemoPatchInit(void)
{
    RECT rc;

    setRECT(&rc, 0x3f0, 0x1ff, 0x10, 1);
    StoreImage2(&rc, (u_long *)DemoBackupArea);
    DrawSync(0);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PutItemCursor(short x, short y, short size, short rotdif);
 *     INFOVIEW.C:358, 5 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short x
 *     param $a1       short y
 *     param $a2       short size
 *     param $a3       short rotdif
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE CursorImage;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

void PutItemCursor(s16 x, s16 y, s16 size, s32 rotdif)
{
    CursorImage.x = x;
    CursorImage.y = y;
    CursorImage.scaley = CursorImage.scalex = size;
    CursorImage.rotate += rotdif;
    GsSortSprite(&CursorImage, OTablePt, 1);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PutItemIcon(int ItemID, short x, short y, short scale);
 *     INFOVIEW.C:349, 7 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int ItemID
 *     param $a1       short x
 *     param $a2       short y
 *     param $a3       short scale
 *
 * Globals it touches, as the original declared them:
 *     extern struct Sprite3D *ItemImage[25];
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

void PutItemIcon(s32 ItemID, short x, short y, short scale)
{
    GsSPRITE *sprite = &ItemImage[ItemID]->sprite;

    sprite->x = x;
    sprite->y = y;
    sprite->scalex = scale;
    sprite->scaley = scale;
    GsSortSprite(sprite, OTablePt, 0);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PutNumber(int x, int y, int cols, int n);
 *     INFOVIEW.C:197, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int x
 *     param $a1       int y
 *     param $a2       int cols
 *     param $a3       int n
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE NumberImage;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

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
