#include "common.h"
#include "../main.exe/tuning.h"
#include "menu.exe.h"
#include "../main.exe/graphics.h"
#include "../main.exe/adt.h"
#include "../main.exe/images.h"
#include "../main.exe/item.h"
#include "../main.exe/model.h"
#include "../main.exe/tim.h"
#include "../main.exe/timpack.h"
#include "../main.exe/tmdfile.h"
#include "../main.exe/vmemory.h"
#include <psxsdk/libgpu.h>

/* The menu shares MAIN's model loaders and coordinate types, and submits
 * models through the SDK renderer with scratchpad workspace. */

extern GsOT_TAG ZSortTable[N_DRAW_PAGES][N_OT_TAGS];
extern u8 Packet[][PACKET_PAGE_SIZE];
extern s32 time;
extern VECTOR vector;

extern char msg_no_model_archive_data[]; /* NO MODEL ARCHIVE DATA */
extern char msg_no_source_model_archive[]; /* NO SOURCE MODEL ARCHIVE DATA */
extern char msg_no_background_image_data[]; /* NO BACKGROUND IMAGE DATA */
extern char msg_no_image_data[]; /* NO IMAGE DATA */
extern char msg_no_image_pack_data[]; /* NO IMAGE PACK DATA */
extern char msg_bad_skip_mode[];

void InitGraphicsSystem(void)
{
    ResetGraph(3);
    GsInitGraph(SCREEN_W, SCREEN_H, 4, 1, 0);
    GsDefDispBuff(0, 0, 0, SCREEN_H);
    GsInit3D();
    GsInitCoordinate2(NULL, &World.locate);
    UpdateCoordinate(&World);
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
}

void EndDrawing(short sync)
{
    s16 sk;
    u32 t;
    s32 dp;

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

    default:
        SkipFrame = 0;
        AdtMessageBox(msg_bad_skip_mode);
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

    return depth;
}

short DrawModel(ModelType *objp)
{
    MATRIX mat;

    GsGetLs(&objp->locate, &mat);
    GsSetLsMatrix(&mat);
    if (GetModelDrawDepth(objp, 0) == MODEL_CLIP_REJECTED)
        return 0;

    GsSortObject4(&objp->object, OTablePt, 2,
                  (u_long *)TENCHU_SCRATCHPAD_ADDRESS);
    return 1;
}

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
                GsSortObject4(&objp->object, OTablePt, 2,
                              (u_long *)TENCHU_SCRATCHPAD_ADDRESS);
            }
        }
    }
    return 1;
}

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

short DrawSprite(Sprite3D *sprt)
{
    MATRIX mat;
    ModelType *objp;
    long *xy;
    long result;
    long pri;
    s32 iv;

    objp = (ModelType *)sprt;
    GsGetLs(&objp->locate, &mat);
    GsSetLsMatrix(&mat);
    xy = (long *)&sprt->sprite.x;
    result = GetModelDrawDepth(objp, xy);
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

void UpdateCoordinate(ModelType *dim)
{
    RotMatrixYXZ(&dim->rotate, &dim->locate.coord);
    dim->locate.flg = 0;
}

/* Rebuild a model's coordinate matrix from its rotate vector and mark the
 * GsCOORDINATE2 dirty. */
void UpdateCoordinate2(ModelType *dim)
{
    RotMatrix(&dim->rotate, &dim->locate.coord);
    dim->locate.flg = 0;
}

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

long DrawClip(ModelType *objp, long *xy)
{
    u16 attr;
    long depth;
    short screen[2];

    attr = objp->attribute;
    if (attr & MODEL_ATTR_HIDDEN)
        return MODEL_CLIP_REJECTED;

    if (!(attr & MODEL_ATTR_NOCULL))
    {
        depth = RotTransPers(&objp->clip, (long *)screen, 0, 0) >> 2;
        if ((attr & MODEL_ATTR_CULL_BEHIND) && depth == 0)
            return MODEL_CLIP_REJECTED;
        if ((attr & MODEL_ATTR_CULL_SCREEN) &&
            (__builtin_abs(screen[0]) > MODEL_CULL_X_LIMIT ||
             __builtin_abs(screen[1]) > MODEL_CULL_Y_LIMIT))
            return MODEL_CLIP_REJECTED;
        if ((attr & MODEL_ATTR_CULL_FAR) && depth > DEPTH_LIMIT)
            return MODEL_CLIP_REJECTED;
    }

    depth = RotTransPers(&UnitVector, xy, 0, 0) >> 2;
    if (depth > DEPTH_LIMIT)
        return MODEL_CLIP_REJECTED;
    return depth;
}

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

void DisposeModel(ModelType *objp)
{
    if (objp != 0)
    {
        vfree(objp);
    }
}

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

short DrawOrnament(OrnamentType *objp)
{
    MATRIX mat;

    GsGetLs(&objp->locate, &mat);
    GsSetLsMatrix(&mat);
    GsSortObject4(&objp->object, OTablePt, 2,
                  (u_long *)TENCHU_SCRATCHPAD_ADDRESS);
    return 1;
}

void DisposeOrnament(OrnamentType *objp)
{
    if (objp != 0)
    {
        vfree(objp);
    }
}

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

void UpdateOrnament(OrnamentType *objp, short ry)
{
    SVECTOR rotv;

    rotv = UnitVector;
    rotv.vy = ry;
    RotMatrixYXZ(&rotv, &objp->locate.coord);
    objp->locate.flg = 0;
}

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

void LoadTIMpackAndFree(u_long *tim)
{
    LoadTIMpack(tim);
    vfree(tim);
}

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
    for (i = 0; i < n; i++, adr++)
    {
        GsGetTimInfo(TIM_PACK_IMAGE(p, adr), &tim);
        setRECT(&rect, tim.px, tim.py, tim.pw, tim.ph);
        LoadImage(&rect, tim.pixel);
        if (TIM_HAS_CLUT(tim.pmode) != 0)
        {
            setRECT(&rect, tim.cx, tim.cy, tim.cw, tim.ch);
            LoadImage(&rect, tim.clut);
        }
    }
    DrawSync(0);
}

s16 GetTIMInfo(u_long *adr, GsIMAGE *image)
{
    GsGetTimInfo(TIM_FILE_IMAGE(adr), image);
    return 1;
}

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

/* MENU string storage. Keep the retail spelling and development paths. */
char msg_bad_skip_mode[] = "bad skip mode";
char msg_no_model_archive_data[] = "NO MODEL ARCHIVE DATA";
char msg_no_source_model_archive[] = "NO SOURCE MODEL ARCHIVE DATA";
char msg_no_background_image_data[] = "NO BACKGROUND IMAGE DATA";
char msg_no_image_data[] = "NO IMAGE DATA";
char msg_no_image_pack_data[] = "NO IMAGE PACK DATA";
