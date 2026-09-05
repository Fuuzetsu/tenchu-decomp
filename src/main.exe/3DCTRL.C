#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "graphics.h"
#include "adt.h"
#include "images.h"
#include "item.h"
#include "model.h"
#include "tim.h"
#include "timpack.h"
#include "tmdfast.h"
#include "tmdfile.h"
#include "vmemory.h"
#include <psxsdk/libgpu.h>

/*
 * Retail 3DCTRL.C has a substantially different function order from the
 * earlier demo. The manifest preserves both orders independently.
 */

extern GsOT_TAG ZSortTable[N_DRAW_PAGES][N_OT_TAGS];
extern s32 SlightPoint;
extern u8 Packet[][PACKET_PAGE_SIZE];
extern u32 PacketUsed;
extern s32 time;
extern VECTOR vector;

extern char msg_no_model_archive_data[]; /* NO MODEL ARCHIVE DATA */
extern char msg_no_source_model_archive[]; /* NO SOURCE MODEL ARCHIVE DATA */
extern char msg_no_background_image_data[]; /* NO BACKGROUND IMAGE DATA */
extern char msg_no_image_data[]; /* NO IMAGE DATA */
extern char msg_no_image_pack_data[]; /* NO IMAGE PACK DATA */

/* Keep this literal until the persistent word's declaration is recovered. */
#define STARTING_RNG_SEED (*(s32 *)TENCHU_PERSISTENT_RNG_ADDRESS)

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitGraphicsSystem(void);
 *     3DCTRL.C:45, 29 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 *     extern struct GsFOGPARAM Fog;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct GsOT OTable[2];
 *     extern struct GsOT_TAG ZSortTable[2][2048];
 * END PSX.SYM */

void InitGraphicsSystem(void)
{
    SetDispMask(0);
    GsInitGraph(SCREEN_W, SCREEN_H, 0x34, 1, 0);
    GsDefDispBuff(0, 0, 0, SCREEN_H);
    GsInit3D();
    GsInitCoordinate2(NULL, &World.locate);
    UpdateCoordinate(&World);
    DrawTMDmode = TMD_BANK_PLAIN;
    SetDepthQ(FOG_DQA, FOG_DQB);
    DepthPoint = DEPTH_LIMIT;
    SlightPoint = 150;
    Fog.dqa = FOG_DQA;
    Fog.dqb = FOG_DQB;
    Fog.bfc = 0;
    Fog.gfc = 0;
    Fog.rfc = 0;
    GsSetFogParam(&Fog);
    GsSetLightMode(0);
    GsSetAmbient(FIXED_HALF, FIXED_HALF, FIXED_HALF);
    GsSetProjection(PROJECTION_DISTANCE);
    ViewInfo.vpx = 0;
    ViewInfo.vpy = 0;
    ViewInfo.vpz = -1000;
    ViewInfo.vrx = 0;
    ViewInfo.vry = 0;
    ViewInfo.vrz = 0;
    ViewInfo.rz = 0;
    ViewInfo.super = &World.locate;
    GsSetRefView2(&ViewInfo);
    GsSetNearClip(0);
    AdtFntLoad(0x3c0, 0x100);
    AdtFntOpen(-(SCREEN_W / 2), -0x68, SCREEN_W, 0xd0, 0, 0x400);
    OTable[1].length = OT_LENGTH;
    OTable[0].length = OT_LENGTH;
    OTable[0].org = ZSortTable[0];
    OTable[1].org = ZSortTable[1];
    STARTING_RNG_SEED += VSync(-1);
    srand(STARTING_RNG_SEED);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void EndDrawing(short sync);
 *     3DCTRL.C:151, 48 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short sync
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern short SkipFrame;
 *     extern unsigned char Packet[2][65536];
 *     extern short DrawingPage;
 *     extern struct GsOT OTable[2];
 *     extern struct GsOT *OTablePt;
 *     extern struct GsFOGPARAM Fog;
 * END PSX.SYM */

void EndDrawing(short sync)
{
    s16 sk;
    u32 t;
    u32 val;
    s32 dp;

    if ((GameClock % 30 == 0) && (SkipFrame == 0))
    {
        val = GsGetWorkBase() - Packet[DrawingPage];
        if (val > PACKET_PAGE_SIZE)
            PacketUsed = PACKET_PAGE_SIZE;
        else
            PacketUsed = val;
    }

    sk = SkipFrame;
    switch (sk)
    {
    case SKIPFRAME_NONE:
        if (VSync(1) > -sync * SCREEN_H - 10)
        {
            SkipFrame = SKIPFRAME_SKIPPED;
            return;
        }
        break;

    case SKIPFRAME_SKIPPED:
        t = sync;
        sync = t << 1;
        SkipFrame = 0;
        dp = sk - (u16)DrawingPage;
        DrawingPage = dp;
        OTablePt = &OTable[DrawingPage];
        break;

    case SKIPFRAME_AFTER_LOAD:
        SkipFrame = 0;
        break;
        }

        OTablePt->org[0x7FE] = OTablePt->org[DEPTH_LIMIT];

        if (sync <= 0)
        {
            DrawSync(0);
            VSync(-sync);
        }
        else
        {
            if (VSync(-1) - time < sync)
                VSync(sync);
            time = VSync(-1);
            ResetGraph(1);
        }

        GsSwapDispBuff();
        GsSortClear(Fog.rfc, Fog.gfc, Fog.bfc, OTablePt);
        GsDrawOt(OTablePt);
    }

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawModel(struct ModelType *objp);
 *     3DCTRL.C:297, 11 src lines, frame 72 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct ModelType * objp
 *     stack sp+16     struct MATRIX mat
 *     reg   $s1       struct ModelType * objp
 *     reg   $v1       long sz
 *     reg   $s0       short atr
 *     stack sp+48     short [2] rxy
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

static inline long GetModelDrawDepth(ModelType *model, long *screen_xy)
{
    ModelAttribute attribute = model->attribute;
    long depth;
    short clip_xy[2];

    if (attribute & MODEL_ATTR_HIDDEN)
        return MODEL_CLIP_REJECTED;

    if (!(attribute & MODEL_ATTR_NOCULL))
    {
        depth = RotTransPers(&model->clip, (s32 *)clip_xy, 0, 0) >> 2;
        if ((attribute & MODEL_ATTR_CULL_BEHIND) && depth == 0)
            return MODEL_CLIP_REJECTED;
        if ((attribute & MODEL_ATTR_CULL_SCREEN) &&
            (__builtin_abs((s32)clip_xy[0]) > MODEL_CULL_X_LIMIT ||
             __builtin_abs((s32)clip_xy[1]) > MODEL_CULL_Y_LIMIT))
            return MODEL_CLIP_REJECTED;
        if ((attribute & MODEL_ATTR_CULL_FAR) && depth > DEPTH_LIMIT)
            return MODEL_CLIP_REJECTED;
    }

    depth = RotTransPers(&UnitVector, screen_xy, 0, 0) >> 2;
    if (depth > DEPTH_LIMIT)
        return MODEL_CLIP_REJECTED;

    if (screen_xy == 0)
    {
        if (depth >= FOG_DEPTH)
            DrawTMDmode = TMD_BANK_FOG;
        else
            DrawTMDmode = TMD_BANK_PLAIN;
    }
    return depth;
}

short DrawModel(ModelType *objp)
{
    MATRIX mat;

    GsGetLs(&objp->locate, &mat);
    GsSetLsMatrix(&mat);
    if (GetModelDrawDepth(objp, 0) == MODEL_CLIP_REJECTED)
        return 0;

    DrawTMD(&objp->object, OTablePt, 0);
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct ModelArchiveType * LoadModelArchive(unsigned long *adr, struct ModelType *prnt);
 *     3DCTRL.C:335, 54 src lines, frame 56 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       unsigned long * adr
 *     param $s6       struct ModelType * prnt
 *     reg   $a3       struct ModelType * dim
 *     reg   $s2       struct ModelArchiveType * mad
 *     reg   $s5       struct ParentingType * prntp
 *     reg   $s7       unsigned char * tmdp
 *     reg   $s3       short i
 *     reg   $v1       short j
 *     reg   $s0       struct ModelType * objp
 *     reg   $s0       struct ModelType * dim
 *     reg   $s2       struct ModelType * dim
 *     reg   $s0       struct ModelType * objp
 *     reg   $s0       struct ModelType * dim
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 * END PSX.SYM */

ModelArchiveType *LoadModelArchive(u_long *adr, ModelType *prnt)
{
    ModelArchiveType *mad;
    ParentingType *prntp;
    u8 *tmdp;
    short i;
    short j;
    short limit;
    u16 count;
    ModelType *dim;
    ModelType *objp;
    GsCOORDINATE2 *super;
    TMDFile *dtmd;
    int parent;

    if (adr == 0)
    {
        SystemOut(msg_no_model_archive_data);
    }
    mad = (ModelArchiveType *)valloc(sizeof(ModelArchiveType));
    adr++;
    mad->n = *(s16 *)adr;
    adr++;
    mad->object = (ModelType **)valloc(mad->n * sizeof(ModelType *));
    prntp = (ParentingType *)adr;
    tmdp = (u8 *)prntp;
    for (i = 0; i < mad->n; i++)
    {
        dtmd = (TMDFile *)(tmdp + prntp[i].index);
        dim = (ModelType *)valloc(sizeof(ModelType));
        if (dtmd != 0)
        {
            GsMapModelingData((u_long *)&dtmd->data);
            GsLinkObject4((u_long)dtmd->data.objects, &dim->object, 0);
        }
        INITIALIZE_MODEL_INSTANCE(dim, &World.locate);
        mad->object[i] = dim;
    }
    if (prnt == 0)
    {
        prnt = &World;
    }
    INITIALIZE_MODEL_STATE(mad, &prnt->locate);
    i = 0;
    count = mad->n;
    if (mad->n > 0)
    {
        do
        {
            objp = mad->object[i];
            super = &mad->locate;
            if (prntp[i].np >= 0 && (j = 0, (s16)count > 0))
            {
                parent = prntp[i].np;
                limit = mad->n;
                do
                {
                    if (parent != prntp[j].nc)
                    {
                        j++;
                    }
                    else
                    {
                        super = &mad->object[j]->locate;
                        break;
                    }
                } while (j < limit);
            }
            GsInitCoordinate2(super, &objp->locate);
            objp->locate.coord.t[0] = prntp[i].dx;
            objp->locate.coord.t[1] = prntp[i].dy;
            objp->locate.coord.t[2] = prntp[i].dz;
            RotMatrixYXZ(&objp->rotate, &objp->locate.coord);
            i++;
            objp->locate.flg = 0;
            count = mad->n;
        } while (i < mad->n);
    }
    mad->rotate.pad = (short)mad->object[0]->locate.coord.t[1];
    return mad;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawModelArchive(struct ModelArchiveType *mad, long gap);
 *     3DCTRL.C:393, 28 src lines, frame 88 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct ModelArchiveType * mad
 *     param $s3       long gap
 *     reg   $s0       struct ModelType * objp
 *     stack sp+16     struct MATRIX mat
 *     stack sp+48     struct SVECTOR pos
 *     reg   $s1       short i
 *     reg   $s2       struct ModelType * objp
 *     reg   $v1       long sz
 *     reg   $s0       short atr
 *     stack sp+56     short [2] rxy
 *
 * Globals it touches, as the original declared them:
 *     extern short SkipFrame;
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

short DrawModelArchive(ModelArchiveType *mad, long gap)
{
    MATRIX mat;
    SVECTOR pos; /* unused in retail, but present in the demo symbols */
    long result;
    short i;
    ModelType *objp;

    if (SkipFrame != SKIPFRAME_SKIPPED)
    {
        if (gap >= 0)
        {
            GsGetLs(&mad->locate, &mat);
            GsSetLsMatrix(&mat);
            result = GetModelDrawDepth((ModelType *)mad, 0);
            if (result + gap < 0)
            {
                return 0;
            }
        }
        for (i = 0; i < mad->n; i++)
        {
            objp = mad->object[i];
            if ((objp->attribute & MODEL_ATTR_HIDDEN) == 0)
            {
                GsGetLs(&objp->locate, &mat);
                GsSetLsMatrix(&mat);
                DrawTMD(&objp->object, OTablePt, gap);
            }
        }
    }
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct ModelArchiveType * CreateCloneModelArchive(struct ModelArchiveType *mad);
 *     3DCTRL.C:438, 27 src lines, frame 48 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s5       struct ModelArchiveType * mad
 *     reg   $s1       struct ModelArchiveType * newmad
 *     reg   $s4       short i
 *     reg   $s1       struct ModelType * dim
 *     reg   $s2       struct ModelType * objp
 *     reg   $s0       struct ModelType * objp
 *     reg   $s0       struct ModelType * dim
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 * END PSX.SYM */

ModelArchiveType *CreateCloneModelArchive(ModelArchiveType *mad)
{
    ModelArchiveType *newmad;
    short i;
    ModelType *objp;
    ModelType *dim;

    if (mad == 0)
    {
        SystemOut(msg_no_source_model_archive);
    }
    newmad = (ModelArchiveType *)valloc(sizeof(ModelArchiveType));
    newmad->n = mad->n;
    newmad->object = (ModelType **)valloc(newmad->n * sizeof(ModelType *));
    INITIALIZE_MODEL_STATE(newmad, mad->locate.super);
    for (i = 0; i < newmad->n; i++)
    {
        objp = mad->object[i];
        dim = (ModelType *)valloc(sizeof(ModelType));
        INITIALIZE_MODEL_INSTANCE(dim, &World.locate);
        if (objp != 0)
        {
            dim->object.tmd = objp->object.tmd;
        }
        newmad->object[i] = dim;
    }
    /* The retail code reads object[0] even when n is zero. */
    newmad->rotate.pad = (short)newmad->object[0]->locate.coord.t[1];
    return newmad;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Sprite3D * SetupSprite(struct Sprite3D *orgsprt, struct GsIMAGE *image);
 *     3DCTRL.C:546, 43 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct Sprite3D * orgsprt
 *     param $s3       struct GsIMAGE * image
 *     reg   $s2       struct Sprite3D * sprt
 *     reg   $s2       struct ModelType * dim
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 * END PSX.SYM */

Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image)
{
    Sprite3D *sprt;
    s32 texture_mode;
    s32 width_shift;

    sprt = (Sprite3D *)valloc(sizeof(Sprite3D));
    if (orgsprt != 0)
    {
        *sprt = *orgsprt;
    }
    else
    {
        ModelType *dim;

        dim = (ModelType *)sprt;
        INITIALIZE_MODEL_STATE(dim, &World.locate);
        sprt->scale = FIXED_ONE;
        sprt->sprite = (GsSPRITE){0};
        sprt->sprite.attribute = 0;
        sprt->sprite.r = sprt->sprite.g = sprt->sprite.b = 0x80;
        sprt->sprite.scalex = sprt->sprite.scaley = FIXED_ONE;
        if (image != 0)
        {
            texture_mode = TIM_PIXEL_MODE((u16)image->pmode);
            sprt->sprite.attribute =
                sprt->sprite.attribute | GS_ATTR_TEXTURE_MODE(texture_mode);
            width_shift = 2 - texture_mode;
            sprt->sprite.w = image->pw << width_shift;
            sprt->sprite.h = image->ph;
            sprt->sprite.tpage =
                GetTPage(texture_mode, 0, image->px, image->py);
            sprt->sprite.u =
                (u8)((image->px << width_shift) &
                     ((1 << (8 - texture_mode)) - 1));
            sprt->sprite.v = (u8)image->py;
            sprt->sprite.cx = image->cx;
            sprt->sprite.cy = image->cy;
            sprt->sprite.mx = sprt->sprite.w >> 1;
            sprt->sprite.my = sprt->sprite.h >> 1;
        }
    }
    return sprt;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawSprite(struct Sprite3D *sprt);
 *     3DCTRL.C:593, 14 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct Sprite3D * sprt
 *     stack sp+16     struct MATRIX mat
 *     reg   $a2       long sz
 *     reg   $s1       struct ModelType * objp
 *     reg   $s2       long * xy
 *     reg   $v1       long sz
 *     reg   $s0       short atr
 *     stack sp+48     short [2] rxy
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

short DrawSprite(Sprite3D *sprt)
{
    MATRIX mat;
    ModelType *objp;
    ModelAttribute atr;
    long *xy;
    long sz;
    long result;
    long pri;
    s32 iv;
    short rxy[2];

    objp = (ModelType *)sprt;
    GsGetLs(&objp->locate, &mat);
    GsSetLsMatrix(&mat);
    atr = objp->attribute;
    xy = (long *)&sprt->sprite.x;
    if ((atr & MODEL_ATTR_HIDDEN) != 0)
        goto reject;
    if ((atr & MODEL_ATTR_NOCULL) == 0)
    {
        sz = RotTransPers(&objp->clip, (s32 *)rxy, 0, 0) >> 2;
        if ((atr & MODEL_ATTR_CULL_BEHIND) != 0 && sz == 0)
        {
            result = MODEL_CLIP_REJECTED;
            goto ret;
        }
        if ((atr & MODEL_ATTR_CULL_SCREEN) != 0)
        {
            iv = rxy[0];
            if (iv < 0)
            {
                iv = -iv;
            }
            if (iv <= MODEL_CULL_X_LIMIT)
            {
                iv = rxy[1];
                if (iv < 0)
                {
                    iv = -iv;
                }
                if (iv > MODEL_CULL_Y_LIMIT)
                    goto reject;
            }
            else
            {
                result = MODEL_CLIP_REJECTED;
                goto ret;
            }
        }
        if ((atr & MODEL_ATTR_CULL_FAR) != 0 && sz > DEPTH_LIMIT)
        {
            result = MODEL_CLIP_REJECTED;
            goto ret;
        }
    }
    sz = RotTransPers(&UnitVector, xy, 0, 0) >> 2;
    if (sz > DEPTH_LIMIT)
    {
    reject:
        result = MODEL_CLIP_REJECTED;
    }
    else
    {
        if (xy == 0)
        {
            if (sz >= FOG_DEPTH)
                DrawTMDmode = TMD_BANK_FOG;
            else
                DrawTMDmode = TMD_BANK_PLAIN;
        }
        result = sz;
    }
ret:
    pri = result - 5;
    if (pri < 1)
    {
        return 0;
    }
    iv = (sprt->scale >> 2) * PROJECTION_DISTANCE;
    sprt->sprite.scalex = sprt->sprite.scaley = (short)(iv / pri);
    GsSortSprite(&sprt->sprite, OTablePt, (u16)pri);
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct BackGround * SetupBG(struct GsIMAGE *image, short w, short h);
 *     3DCTRL.C:622, 60 src lines, frame 96 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param stack+0   struct GsIMAGE * image
 *     param $a1       short w
 *     param $a2       short h
 *     reg   $s4       struct BackGround * bg
 *     reg   $s2       struct GsCELL * cell
 *     reg   $s5       short x
 *     stack sp+16     short y
 *     stack sp+24     short sy
 *     reg   $a1       short n
 *     reg   $s1       short pmode
 *     stack sp+32     short size
 * END PSX.SYM */

BackGround *SetupBG(GsIMAGE *image, short w, short h)
{
    enum
    {
        BACKGROUND_NEUTRAL_COLOR = 0x80,
        BACKGROUND_CELL_SIZE = 16,
        BACKGROUND_INDEX_EMPTY = 0xffff,
    };
    BackGround *bg;
    GsCELL *cell;
    short x;
    short y;
    short sy;
    short n;
    short pmode;
    short size;
    u16 raw_pmode;

    if (image == 0)
        SystemOut((u8 *)msg_no_background_image_data);

    bg = (BackGround *)valloc(sizeof(BackGround));
    bg->id = CONFLICT_NONE;
    bg->attribute = 0;
    bg->hundle = (GsBG){0};

    raw_pmode = image->pmode;
    bg->hundle.r = bg->hundle.g = bg->hundle.b = BACKGROUND_NEUTRAL_COLOR;
    bg->hundle.scalex = bg->hundle.scaley = FIXED_ONE;
    bg->hundle.w = w;
    bg->hundle.h = h;
    bg->hundle.map = &bg->map;
    bg->map.cellw = bg->map.cellh = BACKGROUND_CELL_SIZE;
    bg->map.ncellw = w / bg->map.cellw;
    pmode = TIM_PIXEL_MODE(raw_pmode);
    bg->hundle.attribute = GS_ATTR_TEXTURE_MODE(pmode);
    bg->hundle.mx = bg->hundle.w >> 1;
    bg->hundle.my = bg->hundle.h >> 1;
    bg->map.ncellh = h / bg->map.cellh;

    size = (short)(bg->map.ncellw * bg->map.ncellh);
    bg->map.index = bg->index = (u16 *)valloc(size * sizeof(*bg->index));
    n = 0;
    if (size > 0)
    {
        do
        {
            bg->index[n++] = BACKGROUND_INDEX_EMPTY;
        } while (n < size);
    }

    n = ((u16)image->pw << (2 - pmode)) / bg->map.cellw;
    sy = (short)((u16)image->ph / bg->map.cellh);
    bg->map.base = bg->cell =
        (GsCELL *)valloc(n * sy * sizeof(GsCELL));

    size = (1 << (8 - pmode)) - 1;
    for (y = 0; y < sy; y++)
    {
        for (x = 0; x < n; x++)
        {
            u8 cellw;
            short basepx;
            short py;
            short px;

            cellw = bg->map.cellw;
            py = image->py;
            basepx = image->px;
            px = basepx;
            px += x * (cellw >> (2 - pmode));
            py += y * bg->map.cellh;
            cell = &bg->cell[y * n + x];
            cell->u = ((basepx << (2 - pmode)) +
                       x * cellw) &
                      size;
            cell->v = py;
            cell->cba = GetClut(image->cx, image->cy);
            cell->flag = 0;
            cell->tpage = GetTPage(pmode, 0, px, py);
        }
    }

    bg->work = (u32 *)valloc(
        (s16)((bg->map.ncellw + 1) * (bg->map.ncellh + 2) * 12 + 10) * 4);
    GsInitFixBg16(&bg->hundle, bg->work);
    bg->sz = 0;
    return bg;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void StartDrawing(void);
 *     3DCTRL.C:140, 6 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern short DrawingPage;
 *     extern unsigned char Packet[2][65536];
 *     extern struct GsOT OTable[2];
 *     extern struct GsOT *OTablePt;
 *     extern long GameClock;
 * END PSX.SYM */

void StartDrawing(void)
{
    short newPage;

    newPage = 1 - DrawingPage;
    DrawingPage = newPage;
    GsSetWorkBase(Packet[newPage]);

    OTablePt = &OTable[DrawingPage];
    GsClearOt(0, 0, OTablePt);

    GameClock++;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateCoordinate(struct ModelType *dim);
 *     3DCTRL.C:203, 4 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelType * dim
 * END PSX.SYM */

void UpdateCoordinate(ModelType *dim)
{
    RotMatrixYXZ(&dim->rotate, &dim->locate.coord);
    dim->locate.flg = 0;
}

/* Rebuild a model's coordinate matrix from its rotate vector and mark the
 * GsCOORDINATE2 dirty. */
/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateCoordinate2(struct ModelType *dim);
 *     3DCTRL.C:212, 4 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelType * dim
 * END PSX.SYM */

void UpdateCoordinate2(ModelType *dim)
{
    RotMatrix(&dim->rotate, &dim->locate.coord);
    dim->locate.flg = 0;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct VECTOR * GetAbsolutePosition(struct ModelType *model, short x, short y, short z);
 *     3DCTRL.C:221, 15 src lines, frame 80 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelType * model
 *     param $a1       short x
 *     param $a2       short y
 *     param $a3       short z
 *     stack sp+16     struct MATRIX mat
 *     stack sp+48     struct SVECTOR offset
 * END PSX.SYM */

VECTOR *GetAbsolutePosition(ModelType *model, short x, short y, short z)
{
    MATRIX mat;
    SVECTOR offset;

    GsGetLw(&model->locate, &mat);
    GsSetLsMatrix(&mat);
    offset.vx = x;
    offset.vy = y;
    offset.vz = z;
    RotTrans(&offset, &vector, (long *)0);
    return &vector;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long DrawClip(struct ModelType *objp, long *xy);
 *     3DCTRL.C:240, 23 src lines, frame 40 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelType * objp
 *     param $a1       long * xy
 *     stack sp+16     short [2] rxy
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

long DrawClip(ModelType *objp, long *xy)
{
    u16 attr;
    long sz;
    long result;
    s32 iv;
    short rxy[2];

    attr = objp->attribute;
    if ((attr & MODEL_ATTR_HIDDEN) != 0)
        return MODEL_CLIP_REJECTED;
    if ((attr & MODEL_ATTR_NOCULL) == 0)
    {
        sz = RotTransPers(&objp->clip, (s32 *)rxy, 0, 0) >> 2;
        if ((attr & MODEL_ATTR_CULL_BEHIND) != 0 && sz == 0)
        {
            return MODEL_CLIP_REJECTED;
        }
        if ((attr & MODEL_ATTR_CULL_SCREEN) != 0)
        {
            iv = rxy[0];
            if (iv < 0)
            {
                iv = -iv;
            }
            if (iv <= MODEL_CULL_X_LIMIT)
            {
                iv = rxy[1];
                if (iv < 0)
                {
                    iv = -iv;
                }
                if (iv > MODEL_CULL_Y_LIMIT)
                {
                    goto reject;
                }
            }
            else
            {
                return MODEL_CLIP_REJECTED;
            }
        }
        if ((attr & MODEL_ATTR_CULL_FAR) != 0 && sz > DEPTH_LIMIT)
        {
            return MODEL_CLIP_REJECTED;
        }
    }
    sz = RotTransPers(&UnitVector, xy, 0, 0) >> 2;
    if (sz > DEPTH_LIMIT)
    {
    reject:
        result = MODEL_CLIP_REJECTED;
    }
    else
    {
        if (xy == 0)
        {
            if (sz >= FOG_DEPTH)
                DrawTMDmode = TMD_BANK_FOG;
            else
                DrawTMDmode = TMD_BANK_PLAIN;
        }
        result = sz;
    }
    return result;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct ModelType * LoadModel(unsigned long *adr);
 *     3DCTRL.C:269, 24 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * adr
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 * END PSX.SYM */

ModelType *LoadModel(u_long *adr)
{
    ModelType *model;

    model = (ModelType *)valloc(sizeof(ModelType));
    if (adr != 0)
    {
        adr = (u_long *)&((TMDFile *)adr)->data;
        GsMapModelingData(adr);
        GsLinkObject4((u_long)((TMDData *)adr)->objects, &model->object, 0);
    }
    INITIALIZE_MODEL_INSTANCE(model, &World.locate);
    return model;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeModel(struct ModelType *objp);
 *     3DCTRL.C:312, 2 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelType * objp
 * END PSX.SYM */

void DisposeModel(ModelType *objp)
{
    if (objp != 0)
    {
        vfree(objp);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct ModelType * CreateCloneModel(struct ModelType *objp);
 *     3DCTRL.C:319, 10 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelType * objp
 * END PSX.SYM */

ModelType *CreateCloneModel(ModelType *objp)
{
    ModelType *model;

    model = (ModelType *)valloc(sizeof(ModelType));
    INITIALIZE_MODEL_INSTANCE(model, &World.locate);
    if (objp != 0)
    {
        model->object.tmd = objp->object.tmd;
    }
    return model;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeModelArchive(struct ModelArchiveType *mad);
 *     3DCTRL.C:425, 9 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelArchiveType * mad
 * END PSX.SYM */

void DisposeModelArchive(ModelArchiveType *mad)
{
    s32 i;

    if (mad == 0)
    {
        return;
    }

    for (i = 0; i < mad->n; i++)
    {
        if (mad->object[i] != 0)
        {
            vfree(mad->object[i]);
        }
    }
    vfree(mad->object);
    vfree(mad);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct OrnamentType * LoadOrnament(unsigned long *adr);
 *     3DCTRL.C:471, 21 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * adr
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

OrnamentType *LoadOrnament(u_long *adr)
{
    OrnamentType *ornament;

    ornament = (OrnamentType *)valloc(sizeof(OrnamentType));
    if (adr != 0)
    {
        adr = (u_long *)&((TMDFile *)adr)->data;
        GsMapModelingData(adr);
        GsLinkObject4((u_long)((TMDData *)adr)->objects, &ornament->object, 0);
    }
    INITIALIZE_ORNAMENT_INSTANCE(ornament, &World.locate);
    return ornament;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawOrnament(struct OrnamentType *objp);
 *     3DCTRL.C:496, 10 src lines, frame 56 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct OrnamentType * objp
 *     stack sp+16     struct MATRIX mat
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

short DrawOrnament(OrnamentType *objp)
{
    MATRIX mat;

    GsGetLs(&objp->locate, &mat);
    GsSetLsMatrix(&mat);
    DrawTMD(&objp->object, OTablePt, 0);
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeOrnament(struct OrnamentType *objp);
 *     3DCTRL.C:510, 3 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct OrnamentType * objp
 * END PSX.SYM */

void DisposeOrnament(OrnamentType *objp)
{
    if (objp != 0)
    {
        vfree(objp);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct OrnamentType * CreateCloneOrnament(struct OrnamentType *objp);
 *     3DCTRL.C:518, 10 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct OrnamentType * objp
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

OrnamentType *CreateCloneOrnament(OrnamentType *objp)
{
    OrnamentType *ornament;

    ornament = (OrnamentType *)valloc(sizeof(OrnamentType));
    INITIALIZE_ORNAMENT_INSTANCE(ornament, &World.locate);
    if (objp != 0)
    {
        ornament->object.tmd = objp->object.tmd;
    }
    return ornament;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateOrnament(struct OrnamentType *objp, short ry);
 *     3DCTRL.C:532, 8 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct OrnamentType * objp
 *     param $a1       short ry
 *     stack sp+16     struct SVECTOR rotv
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

void UpdateOrnament(OrnamentType *objp, short ry)
{
    SVECTOR rotv;

    rotv = UnitVector;
    rotv.vy = ry;
    RotMatrixYXZ(&rotv, &objp->locate.coord);
    objp->locate.flg = 0;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawBG(struct BackGround *bg);
 *     3DCTRL.C:686, 6 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct BackGround * bg
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/* Official libgs name: sits immediately before GsInitFixBg16 in the
 * same module order as the demo's GsSortFixBg32/GsInitFixBg32 pair
 * (the demo's DrawBG called the Bg32 variant; retail switched to
 * 16x16 cells). The 4-arg shape is the real implementation ABI. */

short DrawBG(BackGround *bg)
{
    if ((bg->attribute & MODEL_ATTR_HIDDEN) != 0)
    {
        return 0;
    }
    GsSortFixBg16(&bg->hundle, bg->work, OTablePt, bg->sz);
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeBG(struct BackGround *bg);
 *     3DCTRL.C:697, 7 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct BackGround * bg
 * END PSX.SYM */

void DisposeBG(BackGround *bg)
{
    if (bg == 0)
    {
        return;
    }

    vfree(bg->cell);
    vfree(bg->work);
    vfree(bg->index);
    vfree(bg);
}

void LoadTIMAndFree(u_long *tim)
{
    LoadTIM(tim);
    vfree(tim);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * PSX.SYM suggests this may be `ViewAdjustBG` (LOW confidence, 3DCTRL.C) — NOT
 * adopted. Corroborate with `tools/callmatch.py --verify` before renaming.
 * END PSX.SYM */

void LoadTIMpackAndFree(u_long *tim)
{
    LoadTIMpack(tim);
    vfree(tim);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short LoadTIM(unsigned long *adr);
 *     3DCTRL.C:718, 27 src lines, frame 64 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * adr
 *     stack sp+16     struct RECT rect
 *     stack sp+24     struct GsIMAGE tim
 * END PSX.SYM */

short LoadTIM(unsigned long *adr)
{
    RECT rect;
    GsIMAGE tim;

    if (adr == 0)
    {
        SystemOut(msg_no_image_data);
    }
    GsGetTimInfo(TIM_FILE_IMAGE(adr), &tim);
    setRECT(&rect, tim.px, tim.py, tim.pw, tim.ph);
    LoadImage(&rect, tim.pixel);
    if (TIM_HAS_CLUT(tim.pmode))
    {
        setRECT(&rect, tim.cx, tim.cy, tim.cw, tim.ch);
        LoadImage(&rect, tim.clut);
    }
    DrawSync(0);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short LoadTIMpack(unsigned long *adr);
 *     3DCTRL.C:759, 39 src lines, frame 80 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * adr
 *     stack sp+16     struct RECT rect
 *     stack sp+24     struct GsIMAGE tim
 * END PSX.SYM */

short LoadTIMpack(unsigned long *adr)
{
    RECT rect;
    GsIMAGE tim;
    TIMPackIndex *index;
    u_long *p;
    u16 hw;
    short n;
    short i;

    if (adr == 0)
    {
        SystemOut(msg_no_image_pack_data);
    }
    adr++;
    index = (TIMPackIndex *)adr;
    hw = (u16)index->count;
    adr = index->offsets;
    n = (short)hw;
    p = adr;
    for (i = 0; i < n; i++)
    {
        GsGetTimInfo(TIM_PACK_IMAGE(p, adr), &tim);
        setRECT(&rect, tim.px, tim.py, tim.pw, tim.ph);
        LoadImage(&rect, tim.pixel);
        if (TIM_HAS_CLUT(tim.pmode) != 0)
        {
            setRECT(&rect, tim.cx, tim.cy, tim.cw, tim.ch);
            LoadImage(&rect, tim.clut);
            /* Empty loop retained for code layout; its original source construct is unknown. */
            do
            {
            } while (0);
        }
        adr++;
    }
    DrawSync(0);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetTIMInfo(unsigned long *adr, struct GsIMAGE *image);
 *     3DCTRL.C:749, 5 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * adr
 *     param $a1       struct GsIMAGE * image
 * END PSX.SYM */

s16 GetTIMInfo(u_long *adr, GsIMAGE *image)
{
    GsGetTimInfo(TIM_FILE_IMAGE(adr), image);
    return 1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetTIMpackInfo(unsigned long *adr, struct GsIMAGE *image, int idx);
 *     3DCTRL.C:802, 17 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * adr
 *     param $a1       struct GsIMAGE * image
 *     param $a2       int idx
 *     stack sp+16     struct RECT rect
 * END PSX.SYM */

short GetTIMpackInfo(unsigned long *adr, GsIMAGE *image, int idx)
{
    short i;
    TIMPackIndex *index;
    u_long *cursor;
    u_long *offsets;

    adr++;
    index = (TIMPackIndex *)adr;
    if (idx < 0 ||
        (offsets = index->offsets, (int)index->count <= idx))
    {
        return 0;
    }
    cursor = offsets;
    for (i = 0; i < idx; i++)
    {
        cursor++;
    }
    GsGetTimInfo(TIM_PACK_IMAGE(offsets, cursor), image);
    return 1;
}
