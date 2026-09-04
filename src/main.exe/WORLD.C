#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "timpack.h"
#include "tim.h"
#include "graphics.h"
#include "adt.h"
#include "images.h"
#include "appear.h"
#include "chranim.h"
#include "effect.h"
#include "font.h"
#include "item.h"
#include <psxsdk/libgpu.h>
#include "tmdfile.h"
#include "misc.h"
#include "model.h"
#include "sound.h"
#include "vmemory.h"
#include "padcmd.h"
#include "tmdfast.h"
#include "layout_save.h"
#include "memcard.h"
#include "stage.h"

/*
 * PSX.SYM identifies these routines as members of the original WORLD.C.
 * The demo source-line order differs from the shipped retail text order;
 * definitions below follow retail order so the compiler reproduces the
 * executable. Both orders are retained in
 * config/translation-units.main.exe.json.
 */

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void CreateStage(int StageNo, int CharType);
 *     WORLD.C:139, 114 src lines, frame 208 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       int StageNo
 *     param $s5       int CharType
 *     reg   $s0       struct Humanoid * target
 *     reg   $s2       int i
 *     stack sp+24     struct POLY_FT4 ply_ten
 *     stack sp+64     struct POLY_FT4 ply_title1
 *     stack sp+104    struct POLY_FT4 ply_title2
 *     stack sp+144    struct GsIMAGE image
 *     reg   $s1       unsigned long * dat
 *     reg   $s0       struct Humanoid * human
 *     reg   $a0       int i
 *     reg   $s0       void * pBuf
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern int StageID;
 *     extern unsigned char *ImagePath;
 *     extern struct GsOT *OTablePt;
 *     extern short SkipFrame;
 *     extern struct TCameraStatus CamState;
 *     extern enum TSystemFlag SystemFlag;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

extern u8 *TITLE_SPRITES_PTRS[N_LANGUAGES];
extern u8 STAGE_LAYOUT_NUMBER;
extern char fmt_illigal_stage_id[]; /* illigal stage id %d */
extern char path_stage_con[];       /* STAGE.CON */

static void DestroyTraceLine(TraceLine *trace);
static short LoadConstruction(u_long *data);

void CreateStage(stage_id StageNo, int CharType)
{
    Humanoid *target;
    POLY_FT4 ply_ten;
    /* Retained in the original PSX.SYM even though this build no longer uses it. */
    GsIMAGE image;
    u8 *title[N_LANGUAGES];
    TStageConfig *base;
    TStageConfig *stage;
    u_long *dat;
    GsIMAGE *image_info;
    BackGround *bg;
    Humanoid *human;
    int i;
    s32 px;
    s32 py;
    s32 pz;

    if ((u32)StageNo >= N_STAGE_CONFIGS)
    {
        AdtMessageBox(fmt_illigal_stage_id, StageNo);
        return;
    }

    SetDepthQ(FOG_DQA, FOG_DQB);
    DepthPoint = DEPTH_LIMIT;

    while (1)
    {
        if (Humans <= 0)
            break;
        target = HumanGroup[0];
        DestroyTraceLine(target->trace);
        KillHumanoid(target);
    }

    base = StageConfig;
    stage = &base[StageNo];
    ImagePath = stage->path;
    StageID = StageNo;
    SetupSoundEffect(CharType, STAGE_NUMBER(StageNo));
    DoBriefingAndInventorySelection();

    __builtin_memcpy(title, TITLE_SPRITES_PTRS, sizeof(title));
    dat = PathFileRead(ImagePath, title[CHOSEN_LANGUAGE]);
    image_info = GetImage(IMG_TENCHU);
    SetupImageToPolyFT4(image_info, &ply_ten, 0x34, 0x43);
    bg = load_background_(dat);
    vfree(dat);

    clear_screen_();
    StartDrawing();
    GsSortPoly(&ply_ten, OTablePt, 0);
    DrawBG(bg);
    SkipFrame = SKIPFRAME_AFTER_LOAD;
    EndDrawing(0);
    StartDrawing();
    SkipFrame = SKIPFRAME_AFTER_LOAD;
    EndDrawing(0);
    DisposeBG(bg);

    SetupAppearance(CharType, STAGE_NUMBER(StageNo));
    LoadConstruction(PathFileRead(ImagePath, (u8 *)path_stage_con));
    initialise_font();
    InitializeImage();
    ResetInfoview(StageNo);

    human = BreedLife(CharType, 0, 0, 0, 0);
    SetupThinkFunction(human, THINK_MIX_PLAYER);
    human->model->locate.coord.t[0] = stage->px;
    human->model->locate.coord.t[1] = stage->py;
    human->model->locate.coord.t[2] = stage->pz;
    human->model->rotate.vx = 0;
    human->model->rotate.vy = (short)stage->pr;
    human->model->rotate.vz = 0;
    CamState.Owner = human;

    for (i = 0; i < N_LOADOUT_ITEMS; i++)
        human->item[i] =
            ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->selItem[i];

    create_ninken_character_(CharType, StageNo);
    if (((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->layout >= N_STAGE_LAYOUTS)
    {
        ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->layout = rand() % N_STAGE_LAYOUTS;
        SystemFlag |= SYSFLAG_RANDOM_LAYOUT;
    }
    else
    {
        SystemFlag &= ~SYSFLAG_RANDOM_LAYOUT;
    }
    load_layout(STAGE_LAYOUT_NUMBER);
    leLayoutEnemy(ENEMY_LAYOUT_GAMEPLAY);

    px = StageConfig[StageNo].px;
    py = StageConfig[StageNo].py;
    pz = StageConfig[StageNo].pz;
    ViewInfo.vpx = px;
    do
    {
        ViewInfo.vpy = py - 10000;
    } while (0);
    do
    {
        ViewInfo.vpz = pz;
    } while (0);
    ViewInfo.vrx = px;
    ViewInfo.vry = py;
    ViewInfo.vrz = pz;

    StartDrawing();
    CVAsetup();
    SetupStageSequence();
    PadProc();
    EndDrawing(0);
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct OrnamentArchiveType * LoadOrnamentArchive(unsigned long *adr, struct ModelType *prnt);
 *     WORLD.C:259, 57 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       unsigned long * adr
 *     param $s4       struct ModelType * prnt
 *     reg   $a3       struct ModelType * dim
 *     reg   $s1       struct OrnamentArchiveType * mad
 *     reg   $s3       struct ParentingType * prntp
 *     reg   $s5       unsigned char * tmdp
 *     reg   $s2       short i
 *     reg   $v1       short j
 *     reg   $s0       struct OrnamentType * objp
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 * END PSX.SYM */

extern char msg_no_model_archive_data_2[]; /* NO MODEL ARCHIVE DATA */

OrnamentArchiveType *LoadOrnamentArchive(u_long *adr, ModelType *prnt)
{
    OrnamentArchiveType *mad;
    ParentingType *prntp;
    u8 *tmdp;
    short i;
    short j;
    OrnamentType *objp;
    GsCOORDINATE2 *super;
    u32 uncachedSegment;
    int parent;
    int count;

    if (adr == 0)
    {
        SystemOut(msg_no_model_archive_data_2);
    }
    mad = (OrnamentArchiveType *)valloc(sizeof(OrnamentArchiveType));
    mad->data = adr;
    adr++;
    mad->n = *(u16 *)adr;
    adr++;
    i = 0;
    uncachedSegment = PSX_KSEG1_BASE;
    mad->object =
        (OrnamentType **)valloc(mad->n * sizeof(OrnamentType *));
    prntp = (ParentingType *)adr;
    tmdp = (u8 *)adr;

    while (1)
    {
        int idx = i;

        if (idx >= mad->n)
            break;
        i++;
        objp = LoadOrnament((u_long *)(tmdp + prntp[idx].index));
        mad->object[idx] = (OrnamentType *)((u32)objp | uncachedSegment);
    }

    if (prnt == 0)
    {
        prnt = &World;
    }
    GsInitCoordinate2(&prnt->locate, &mad->locate);
    mad->locate.coord.t[0] = 0;
    mad->locate.coord.t[1] = 0;
    mad->locate.coord.t[2] = 0;
    mad->rotate.vx = 0;
    mad->rotate.vy = 0;
    mad->rotate.vz = 0;
    UpdateCoordinate((ModelType *)mad);
    i = 0;
    mad->id = CONFLICT_NONE;
    mad->attribute = 0;
    while (1)
    {
        if (i >= (count = mad->n))
            break;
        objp = mad->object[i];
        super = &mad->locate;
        if (prntp[i].np >= 0 && count > 0)
        {
            j = 0;
            parent = prntp[i].np;
            while (1)
            {
                if (parent == prntp[j].nc)
                {
                    super = &mad->object[j]->locate;
                    break;
                }
                j++;
                if (j >= count)
                    break;
            }
        }
        GsInitCoordinate2(super, &objp->locate);
        objp->locate.coord.t[0] = prntp[i].dx;
        objp->locate.coord.t[1] = prntp[i].dy;
        objp->locate.coord.t[2] = prntp[i].dz;
        UpdateOrnament(objp, 0);
        i++;
        objp->object.attribute |= GS_DOBJ_DIVISION_DEPTH_BITS(2);
    }

    mad->rotate.pad = (short)mad->object[0]->locate.coord.t[1];
    return mad;
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void GetCenterAndSize(unsigned long *tmd, struct SVECTOR *center, int *size);
 *     WORLD.C:390, 40 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * tmd
 *     param $a1       struct SVECTOR * center
 *     param $a2       int * size
 *     reg   $a0       struct SVECTOR * vert
 *     reg   $t7       int nVert
 *     reg   $t0       int i
 *     reg   $t6       short minx
 *     reg   $t2       short miny
 *     reg   $t5       short minz
 *     reg   $t4       short maxx
 *     reg   $t1       short maxy
 *     reg   $t3       short maxz
 *     reg   $a0       short dm
 * END PSX.SYM */

static void GetCenterAndSize(TmdObjectRecord *tmd, SVECTOR *center, int *size)
{
    VERT *vert;
    int nVert;
    int i;
    short minx, miny, minz, maxx, maxy, maxz;
    short dz;
    short dm;

    minx = 0;
    miny = minx;
    minz = minx;
    maxx = minx;
    maxy = minx;
    maxz = minx;

    nVert = tmd->linked.vertex_count;
    vert = tmd->linked.vertices;

    for (i = 0; i < nVert; i++)
    {
        if (maxx < vert[i].vx)
            maxx = vert[i].vx;
        else if (vert[i].vx < minx)
            minx = vert[i].vx;

        if (maxy < vert[i].vy)
            maxy = vert[i].vy;
        else if (vert[i].vy < miny)
            miny = vert[i].vy;

        if (maxz < vert[i].vz)
            maxz = vert[i].vz;
        else if (vert[i].vz < minz)
            minz = vert[i].vz;
    }

    dm = (short)(maxx - minx);
    dz = maxz - minz;
    setVector(center, (maxx + minx) / 2, (maxy + miny) / 2, (maxz + minz) / 2);
    if (dz < (short)(maxy - miny))
        dz = maxy - miny;
    if (dz < dm)
        dz = maxx - minx;
    *size = dz / 2;
}


/* Shared map-coordinate conversion used by both construction passes. */
#define WORLD_CELL(src, out)                                                  \
    {                                                                         \
        long a = src;                                                         \
        long q;                                                               \
                                                                              \
        if (a >= 0)                                                           \
            q = a / CONSTRUCTION_CELL;                                        \
        else                                                                  \
            q = a / CONSTRUCTION_CELL - 1;                                    \
        out = q & WORLD_MAP_AXIS_MASK;                                        \
    }

/* Shared archive disposal used for the mission and common archives. */
#define DISPOSE_ORNAMENT_ARCHIVE(arc)                                         \
    {                                                                         \
        OrnamentArchiveType *mad;                                             \
        int i;                                                                \
                                                                              \
        mad = (arc);                                                          \
        if (mad != 0)                                                         \
        {                                                                     \
            for (i = 0; i < mad->n; i++)                                      \
                DisposeOrnament(mad->object[i]);                              \
            vfree(mad->object);                                               \
            vfree(mad->data);                                                 \
            vfree(mad);                                                       \
        }                                                                     \
    }

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short LoadConstruction(unsigned long *data);
 *     WORLD.C:436, 191 src lines, frame 416 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param stack+0   unsigned long * data
 *     stack sp+352    int ObjectID
 *     stack sp+356    unsigned long * MapModel
 *     stack sp+360    struct WorldDataType * wlddt
 *     reg   $s2       struct OrnamentType * model
 *     reg   $s4       long x
 *     reg   $s3       long y
 *     reg   $s0       long z
 *     stack sp+364    long n
 *     reg   $s5       long i
 *     stack sp+32     unsigned char [256] name
 *     reg   $s3       int nModel
 *     reg   $s1       struct OrnamentArchiveType * mad
 *     reg   $s0       int i
 *     reg   $s1       struct OrnamentArchiveType * mad
 *     reg   $s0       int i
 *     reg   $s3       int n
 *     reg   $s0       int i
 *     reg   $a0       long a
 *     reg   $a0       long a
 *     reg   $a0       long a
 *     stack sp+288    struct SVECTOR center
 *     stack sp+296    int size
 *     reg   $s3       struct tag_ObjectSlotType ** slot
 *     reg   $s2       struct OrnamentType * model
 *     reg   $s6       short shifty
 *     stack sp+304    struct PARAM_ITEM_STAY param
 *     reg   $s6       int i
 *     stack sp+368    struct ParentingType * ix
 *     reg   $fp       int msize
 *     reg   $a0       long a
 *     reg   $a0       long a
 *     reg   $a0       long a
 *     reg   $s3       struct tag_ObjectSlotType ** slot
 *     reg   $s1       struct OrnamentType * model
 *
 * Globals it touches, as the original declared them:
 *     extern struct WorldType WorldMap[8][8][8];
 *     extern unsigned long *GlobalAreaMap;
 *     extern unsigned char *ImagePath;
 *     extern struct ObjectSlotManager ModelSlot;
 *     extern int StageID;
 *     extern struct ModelType World;
 * END PSX.SYM */

/* WorldDataType.mode says which kind of record this row is, not a phase:
 * the values are the stage-data file's own tags and are not contiguous. */
typedef s16 world_record_kind;
enum world_record_kind
{
    WLD_RECORD_AREAMAP = 0, /* .acm collision/height map */
    WLD_RECORD_OBJECT = 2,  /* a placed model, or a clone of an earlier one */
    WLD_RECORD_ENEMY = 3,   /* BreedLife a character at the row's transform */
    WLD_RECORD_ITEM = 4,    /* ReqItemStay a pickup at the row's position */
    WLD_RECORD_TIM = 5,     /* a texture to upload and free */
    WLD_RECORD_TIM_PACK = 6, /* the authored "TIM" pack marker */
    WLD_RECORD_EFFECT = 11  /* AddMisc, with the row's three extra params */
};

enum
{
    WORLD_RESOURCE_NAME_SIZE = 12
};

typedef struct WorldTransform
{
    s32 x;
    s32 y;
    s32 z;
    s32 r;
} WorldTransform;

typedef struct WorldCommonPayload
{
    u8 name[WORLD_RESOURCE_NAME_SIZE];
    s32 x;
    s32 y;
    s32 z;
    s32 r;
} WorldCommonPayload;

/* A named object is replaced in place by its loaded pointer. Empty names
 * instead select an earlier row through the record id. */
typedef union WorldObjectSource
{
    u8 name[WORLD_RESOURCE_NAME_SIZE];
    OrnamentType *model;
} WorldObjectSource;

typedef struct WorldObjectPayload
{
    WorldObjectSource source;
    WorldTransform transform;
} WorldObjectPayload;

typedef struct WorldResourcePayload
{
    u8 name[WORLD_RESOURCE_NAME_SIZE];
    s32 reserved[4];
} WorldResourcePayload;

typedef struct WorldPlacementPayload
{
    u8 reserved[WORLD_RESOURCE_NAME_SIZE];
    WorldTransform transform;
} WorldPlacementPayload;

typedef struct WorldEffectPayload
{
    MiscType type;
    s32 x;
    s32 y;
    s32 z;
    MiscSpawnParameters parameters;
} WorldEffectPayload;

typedef struct WorldDataType
{
    world_record_kind mode;
    s16 id; /* clone row, character kind, or item kind, selected by mode */
    union
    {
        WorldCommonPayload common; /* PSX.SYM's original raw field view */
        WorldObjectPayload object;
        WorldResourcePayload resource;
        WorldPlacementPayload placement;
        s32 pathxz[7];
        u8 data[28];
        WorldEffectPayload effect;
    } real;
} WorldDataType;

extern ObjectSlotManager ModelSlot;
extern OrnamentArchiveType *mma;
extern OrnamentArchiveType *ObjectArc;

extern char msg_modelslot_overflow[];   /* ModelSlot Overflow */
extern char msg_no_construction_data[]; /* NO CONSTRUCTION DATA */
extern char path_image[];               /* K:\\WORK\\CDIMAGE\\IMAGE\\ */
extern char path_common_tpd[];          /* COMMON.TPD */
extern char path_objects_mad[];         /* OBJECTS.MAD */
extern char path_balmer_acm[];          /* BALMER.ACM */
extern char path_tim_tpd[];             /* TIM.TPD */
extern char fmt_acm[];                  /* %s.ACM */
extern char fmt_tim[];                  /* %s.TIM */
extern char path_map_mad[];             /* map.mad */

static short LoadConstruction(u_long *data)
{
    enum
    {
        MapModelSize = 0x6B800
    };
    int ObjectID;
    u_long *MapModel;
    WorldDataType *wlddt;
    OrnamentType *model;
    long x;
    long y;
    long z;
    long n;
    long i;
    int nModel;
    short shifty;
    unsigned char name[256];
    SVECTOR center;
    PARAM_ITEM_STAY param;
    int size;

    ObjectID = 0;
    wlddt = (WorldDataType *)data;
    if (wlddt == 0)
        SystemOut(msg_no_construction_data);
    memset(WorldMap, 0, sizeof(WorldMap));

    nModel = 0;
    n = vsize(data) / sizeof(WorldDataType);
    i = nModel;
    if (n != 0)
    {
        do
        {
            if (wlddt[i].mode == WLD_RECORD_OBJECT)
                nModel++;
            i++;
        } while (i < n);
    }

    DisposeAreaMap(GlobalAreaMap);
    GlobalAreaMap = 0;
    ResetAllMisc();
    ClearItemLayout();

    DISPOSE_ORNAMENT_ARCHIVE(mma);

    DISPOSE_ORNAMENT_ARCHIVE(ObjectArc);

    LoadTIMpackAndFree(PathFileRead(ImagePath, (u8 *)path_tim_tpd));
    LoadTIMpackAndFree(PathFileRead((u8 *)path_image,
                                    (u8 *)path_common_tpd));

    MapModel = (u_long *)valloc(MapModelSize);
    ObjectArc = LoadOrnamentArchive(
        PathFileRead(ImagePath, (u8 *)path_objects_mad), &World);

    nModel += 500;
    {
        OrnamentType *disposeModel;
        int i;
        ObjectSlotManager *slotman;

        slotman = &ModelSlot;
        if (slotman->max > 0)
        {
            for (i = 0; i < slotman->n; i++)
            {
                disposeModel = slotman->slot[i].model;
                /* Only dispose a slot that holds a cached KSEG0 pointer;
                 * the others carry sentinels. */
                if (((u32)disposeModel & PSX_ADDRESS_REGION_MASK) ==
                    PSX_KSEG0_BASE)
                    DisposeOrnament(disposeModel);
            }
            vfree(slotman->slot);
        }

        slotman->max = nModel;
        slotman->slot = (ObjectSlotType *)valloc(nModel * sizeof(ObjectSlotType));
        slotman->n = 0;
    }

    {
        int msize;
        ObjectSlotType **slot;
        ObjectSlotManager *slotman;

        i = 0;
        while (1)
        {
            if (i >= n)
                break;
            switch (wlddt[i].mode)
            {
            case WLD_RECORD_AREAMAP:
                sprintf((char *)name, fmt_acm,
                        wlddt[i].real.resource.name);
                DisposeAreaMap(GlobalAreaMap);
                GlobalAreaMap = LoadAreaMap(PathFileRead(ImagePath, name));
                if (StageID == STAGE_ID_PIRATES)
                {
                    BalmaAreaMap = load_balma_area_map_(
                        PathFileRead(ImagePath, (u8 *)path_balmer_acm));
                }
                break;

            case WLD_RECORD_TIM:
                sprintf((char *)name, fmt_tim,
                        wlddt[i].real.resource.name);
                LoadTIMAndFree(PathFileRead((u8 *)path_image, name));
                break;

            case WLD_RECORD_TIM_PACK:
                break;

            case WLD_RECORD_OBJECT:
                if (wlddt[i].real.object.source.name[0] != 0)
                {
                    model = ObjectArc->object[ObjectID];
                    ObjectID++;
                    wlddt[i].real.object.source.model = model;
                }
                else
                    model = CreateCloneOrnament(
                        wlddt[wlddt[i].id]
                            .real.object.source.model);

                model->locate.coord.t[0] =
                    wlddt[i].real.object.transform.x;
                model->locate.coord.t[1] =
                    wlddt[i].real.object.transform.y;
                model->locate.coord.t[2] =
                    wlddt[i].real.object.transform.z;
                UpdateOrnament(model, wlddt[i].real.object.transform.r);

                WORLD_CELL(wlddt[i].real.object.transform.x, x);
                WORLD_CELL(wlddt[i].real.object.transform.y, y);
                WORLD_CELL(wlddt[i].real.object.transform.z, z);

                GetCenterAndSize((TmdObjectRecord *)model->object.tmd,
                                 &center, &size);
                slot = (ObjectSlotType **)WORLD_MAP_CELL_BYTE_OFFSET(x, y, z);
                slot = (ObjectSlotType **)((u8 *)WorldMap + (u32)slot);
                slotman = &ModelSlot;
                shifty = center.vy;
                msize = size / 2;
                if (slotman->n >= slotman->max)
                    AdtMessageBox(msg_modelslot_overflow);
                slotman->slot[slotman->n].model = model;
                slotman->slot[slotman->n].next = *slot;
                slotman->slot[slotman->n].ModelSize = msize;
                slotman->slot[slotman->n].ShiftY = shifty;
                *slot = &slotman->slot[slotman->n];
                slotman->n++;
                break;

            case WLD_RECORD_ENEMY:
                BreedLife(wlddt[i].id,
                          wlddt[i].real.placement.transform.x,
                          wlddt[i].real.placement.transform.y,
                          wlddt[i].real.placement.transform.z,
                          wlddt[i].real.placement.transform.r);
                break;

            case WLD_RECORD_EFFECT:
                AddMisc(wlddt[i].real.effect.type, wlddt[i].real.effect.x,
                        wlddt[i].real.effect.y, wlddt[i].real.effect.z,
                        wlddt[i].real.effect.parameters.a,
                        wlddt[i].real.effect.parameters.b,
                        wlddt[i].real.effect.parameters.c);
                break;

            case WLD_RECORD_ITEM:
            {
                PARAM_ITEM_STAY tmp = {0};

                tmp.type = wlddt[i].id;
                tmp.locate.vx = wlddt[i].real.placement.transform.x;
                tmp.locate.vy = wlddt[i].real.placement.transform.y;
                tmp.locate.vz = wlddt[i].real.placement.transform.z;
                param = tmp;
                ReqItemStay(&param);
                break;
            }
            }
            i++;
        }
    }

    {
        OrnamentType *model;
        int i;
        int parent;
        ObjectSlotType **slot;
        ObjectSlotManager *slotman;
        ParentingType *ix;
        int msize;

        sprintf((char *)name, path_map_mad);
        vfree(MapModel);
        i = 0;
        MapModel = PathFileRead(ImagePath, name);
        ix = ((ModelArchiveFile *)MapModel)->parenting;
        mma = LoadOrnamentArchive(MapModel, &World);

        while (1)
        {
            if (i >= mma->n)
                break;
            parent = ix[i].np;
            mma->object[i]->locate.coord.t[0] *= 10;
            mma->object[i]->locate.coord.t[1] *= 10;
            mma->object[i]->locate.coord.t[2] *= 10;

            {
                long a = mma->object[i]->locate.coord.t[0];
                long q;

                msize = -parent * 14;
                if (a >= 0)
                    q = a / CONSTRUCTION_CELL;
                else
                    q = a / CONSTRUCTION_CELL - 1;
                x = q & 7;
            }
            WORLD_CELL(mma->object[i]->locate.coord.t[1], y);
            WORLD_CELL(mma->object[i]->locate.coord.t[2], z);

            mma->object[i]->object.attribute |=
                GS_DOBJ_DIVISION_DEPTH_BITS(2);
            UpdateOrnament(mma->object[i], 0);
            slot = (ObjectSlotType **)WORLD_MAP_CELL_BYTE_OFFSET(x, y, z);
            slot = (ObjectSlotType **)((u8 *)WorldMap + (u32)slot);
            slotman = &ModelSlot;
            model = mma->object[i];
            if (slotman->n >= slotman->max)
                AdtMessageBox(msg_modelslot_overflow);
            slotman->slot[slotman->n].model = model;
            slotman->slot[slotman->n].next = *slot;
            slotman->slot[slotman->n].ModelSize = msize;
            slotman->slot[slotman->n].ShiftY = 0;
            *slot = &slotman->slot[slotman->n];
            slotman->n++;
            i++;
        }
    }

    vfree(data);
    jt_init4();
    return -1;
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int IsVisible(long x, long y, long z, long s);
 *     WORLD.C:685, 63 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long x
 *     param $a1       long y
 *     param $a2       long z
 *     param $a3       long s
 * END PSX.SYM */


int IsVisible(s32 x, s32 y, s32 z, s32 s)
{
    enum
    {
        SXW = 160,
        SYW = 120
    };
    enum
    {
        NEAR = 150
    };
    GsRVIEW2 *view;
    VECTOR *view_space;
    s32 dx, dy, dz;
    s32 zs;
    s32 aq;
    s32 qs;
    s32 q0, q2;
    s32 fail;

    view = CONSTRUCTION_VISIBILITY_VIEW;

    dx = x - view->vpx;
    if (30000 < abs(dx))
        return 0;

    dy = y - view->vpy;
    if (30000 < abs(dy))
        return 0;

    dz = z - view->vpz;
    if (30000 < abs(dz))
        return 0;

    CONSTRUCTION_VISIBILITY_RELATIVE->vx = (s16)dx;
    CONSTRUCTION_VISIBILITY_RELATIVE->vy = (s16)dy;
    CONSTRUCTION_VISIBILITY_RELATIVE->vz = (s16)dz;
    ApplyRotMatrix(CONSTRUCTION_VISIBILITY_RELATIVE,
                   CONSTRUCTION_VISIBILITY_VIEW_SPACE);

    view_space = CONSTRUCTION_VISIBILITY_VIEW_SPACE;
    zs = view_space->vz + s;
    if (zs <= NEAR)
        return 0;
    if (17000 < view_space->vz - s)
        return 0;

    q0 = (view_space->vx * PROJECTION_DISTANCE) / zs;
    qs = (s * PROJECTION_DISTANCE) / zs;
    q2 = (view_space->vy * PROJECTION_DISTANCE) / zs;
    fail = 0;
    aq = abs(q0);
    if (qs + SXW < aq || qs + SYW < abs(q2))
        fail = 1;
    return !fail;
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActivateHumans(void);
 *     WORLD.C:752, 41 src lines, frame 48 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       int i
 *     reg   $s0       struct Humanoid * target
 *     stack sp+16     struct VECTOR vc
 *     reg   $s0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct Humanoid *StagePlayer;
 *     extern long GameClock;
 *     extern short SkipFrame;
 *     extern struct StageCharType StageChar[18];
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern int StageID;
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

extern s32 PacketUsed; /* This TU uses a signed view of the u32 definition. */
extern s16 ThinkBudgetRaw;
extern s16 ThinkCount;
extern s16 ThinkBudget;

/* One thinking human costs about this many GPU packet bytes, and the
 * budget keeps PacketUsed a whole unit clear of the end of a Packet[]
 * row: 0x10000 - THINK_PACKET_COST == 0xec78. */
#define THINK_PACKET_COST 5000
#define THINK_PACKET_LIMIT (0x10000 - THINK_PACKET_COST)

void ActivateHumans(void)
{
    s32 i;
    Humanoid *target;
    VECTOR vc;
    s32 activate_distance;

    target = CamState.Owner;
    vc = *target->locate;
    activate_distance = ACTIVATE_RADIUS_WIDE;
    if (StagePlayer->motion->mid != MOT_ITEM_SHINSOKU)
    {
        activate_distance = ACTIVATE_RADIUS;
    }

    if (GameClock % 30 != 0 || SkipFrame != 0)
    {
        return;
    }

    /* GPU-packet headroom: how many more humans may think this frame. */
    ThinkBudgetRaw = (THINK_PACKET_LIMIT - PacketUsed) / THINK_PACKET_COST - 1;
    ThinkBudget = ThinkBudgetRaw < 2
                      ? (u16)ThinkBudget - 1
                      : ThinkBudgetRaw;
    /* Clamp the usable budget to 3..6. */
    ThinkBudget = ThinkBudget < 7
                      ? (ThinkBudget < 3 ? 3 : ThinkBudget)
                      : 6;
    i = 0;
    ThinkCount = 0;
    while (1)
    {
        Humanoid *human;

        if ((s16)i >= Humans)
        {
            return;
        }
        human = HumanGroup[(s16)i];
        if (human != target)
        {
            s32 active;
            s32 final;
            s32 distance;
            s16 j;

            distance = GetVectorDistance(human->locate, &vc);
            if (distance > DEACTIVATE_RADIUS)
            {
                active = 0;
            }
            else if (((u16)human->type & PAGE_MASK) == PAGE_BOSS)
            {
                active = 1;
            }
            else if (human->type == NINKEN || human->life < 0)
            {
                active = 1;
            }
            else if (GameClock == 30 || StageID == STAGE_ID_TRAINING)
            {
                active = 1;
            }
            else
            {
                if (VISIBLE_ENEMIES_ < ThinkBudget)
                {
                    active = 1;
                    if (ThinkCount >= ThinkBudget)
                    {
                        final = distance < activate_distance;
                        goto visible_done;
                    }
                }
                else if (distance >= activate_distance)
                {
                    active = 0;
                }
                else if (((u16)human->attribute & ATTR_SUSPEND) == 0 &&
                         ThinkCount < ThinkBudget)
                {
                    active = 1;
                }
                else
                {
                    j = 0;
                    while (VISIBLE_CHARACTERS_ON_STAGE_[j] != human)
                    {
                        if (VISIBLE_ENEMIES_ <= j)
                        {
                            break;
                        }
                        j++;
                    }
                    final = j != VISIBLE_ENEMIES_;

                visible_done:
                    /* This earlier `final` lifetime ends at the visibility join; the
                     * active-result join below overwrites it before its next use. */
                    active = final;
                }
            }
            if (human)
            {
                final = 0;
                final = active;
            }
            else
            {
                final = active;
            }
            if (final)
            {
                if (((u16)human->attribute & ATTR_SUSPEND) == 0)
                {
                    ThinkCount++;
                }
                else if (StageID == STAGE_ID_TRAINING || human->life < 0 || GameClock == 30 ||
                         (ThinkCount < ThinkBudget && distance > ACTIVATE_RADIUS))
                {
                    human->attribute = (u16)human->attribute & ~ATTR_SUSPEND;
                    ThinkCount++;
                    (*human->model->object)->attribute |= MODEL_ATTR_COLLIDE;
                }
            }
            else if (((u16)human->attribute & ATTR_SUSPEND) == 0 && human->type != ON)
            {
                if ((human->type == NINJA_0 &&
                     (u32)(StageID - STAGE_ID_RECLAIM_CASTLE) <=
                         STAGE_ID_FREE_PRINCESS - STAGE_ID_RECLAIM_CASTLE) ||
                    human->type == GOO)
                {
                    j = 0;
                    while (StageChar[j].stage != STAGE_CHAR_END)
                    {
                        if (StageChar[j].stage == STAGE_NUMBER(StageID) &&
                            StageChar[j].chrid == human->type)
                        {
                            human->model->locate.coord.t[0] = StageChar[j].position.vx * 1000;
                            human->model->locate.coord.t[1] = StageChar[j].position.vy * 1000;
                            human->model->locate.coord.t[2] = StageChar[j].position.vz * 1000;
                        }
                        j++;
                    }
                    if (human->type == GOO && human->life == 0)
                    {
                        human->life = 1;
                    }
                }
                else if (human->status != STAT_DEAD && ((u16)human->attribute & ATTR_FLOAT) == 0)
                {
                    VECTOR query = {
                        .vx = human->point[HUMANOID_HOME_X],
                        .vy = human->locate->vy - 1500,
                        .vz = human->point[HUMANOID_HOME_Z]
                    };
                    s32 level;

                    if (GetVectorDistance(&query, &vc) > DEACTIVATE_RADIUS)
                    {
                        level = GetAreaMapLevel(GlobalAreaMap, query.vx, query.vy,
                                                query.vz, AREA_LEVEL_STEP_DOWN);
                        if (level != LEVEL_NONE)
                        {
                            human->model->locate.coord.t[0] = query.vx;
                            human->model->locate.coord.t[1] = level;
                            human->model->locate.coord.t[2] = query.vz;
                        }
                    }
                }

                human->attribute = (u16)human->attribute | ATTR_SUSPEND;
                (*human->model->object)->attribute &= ~MODEL_ATTR_COLLIDE;
            }
        }

        i++;
    }
}


enum construction_draw_geometry
{
    N_DRAW_BUCKETS = 153,
    N_DRAW_SLOTS = 100,
    CONSTRUCTION_CELL_CENTER_OFFSET = CONSTRUCTION_CELL / 2,
    CONSTRUCTION_SCAN_RADIUS_XZ = 2,
    CONSTRUCTION_SCAN_RADIUS_Y = 1,
    CONSTRUCTION_CELL_VISIBILITY_RADIUS = 0x2BC1,
    CONSTRUCTION_DEPTH_BUCKET_SHIFT = 8,
    CONSTRUCTION_DEPTH_BUCKET_BIAS = 11,
    CONSTRUCTION_LOCAL_OT_OFFSET = 0x37
};

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawConstruction(void);
 *     WORLD.C:798, 234 src lines, frame 1920 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s6       short j
 *     reg   $s4       short k
 *     reg   $s2       unsigned long plimit
 *     reg   $a2       int nx
 *     reg   $a1       int ny
 *     reg   $a0       int nz
 *     reg   $a0       long a
 *     reg   $a0       long a
 *     reg   $a0       long a
 *     stack sp+1848   int ndl
 *     stack sp+1852   int ndt
 *     stack sp+16     struct tag_ObjectSlotType *[153] DrawList
 *     stack sp+632    struct tag_ObjectSlotType [100] Slot
 *     stack sp+1832   struct ObjectSlotManager SlotMan
 *     reg   $s0       int sx
 *     stack sp+1856   int sy
 *     stack sp+1860   int sz
 *     stack sp+1864   int ex
 *     stack sp+1868   int ey
 *     stack sp+1872   int ez
 *     reg   $s3       int i
 *     reg   $s1       struct tag_ObjectSlotType * cur
 *     reg   $v0       long y
 *     reg   $v0       long z
 *     reg   $a1       int sz
 *     reg   $s3       long a
 *     reg   $v0       long x
 *     reg   $a3       long y
 *     reg   $a2       long z
 *     reg   $a1       int sz
 *     reg   $v0       int k
 *     reg   $s2       struct tag_ObjectSlotType ** slot
 *     reg   $s0       struct OrnamentType * model
 *     reg   $s0       struct tag_ObjectSlotType * cur
 *     reg   $s0       struct tag_ObjectSlotType * cur
 *
 * Globals it touches, as the original declared them:
 *     extern short SkipFrame;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct WorldType WorldMap[8][8][8];
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

extern char msg_overload[];
extern char fmt_objs_d[]; /* objs D%d/%d; */
extern char fmt_pk_size[];
extern char str_map[]; /* map: */
extern char str_newline_2[];

void DrawConstruction(void)
{
    short j;
    short k;
    short l;
    unsigned long plimit;
    int nx;
    int ny;
    int nz;
    int ndl;
    int ndt;
    ObjectSlotType *DrawList[N_DRAW_BUCKETS];
    ObjectSlotType Slot[N_DRAW_SLOTS];
    ObjectSlotManager SlotMan;
    int sx;
    int sy;
    int sz;
    int ex;
    int ey;
    int ez;
    ObjectSlotType *cur;

    if (SkipFrame == SKIPFRAME_SKIPPED)
        return;

    {
        long a = ViewInfo.vpx;

        if (a >= 0)
            nx = a / CONSTRUCTION_CELL;
        else
            nx = a / CONSTRUCTION_CELL - 1;
    }
    {
        long a = ViewInfo.vpy;

        if (a >= 0)
            ny = a / CONSTRUCTION_CELL;
        else
            ny = a / CONSTRUCTION_CELL - 1;
    }
    {
        long a = ViewInfo.vpz;

        if (a < 0)
            goto negative_z;
        nz = a / CONSTRUCTION_CELL;
        goto have_z;
overload:
        FntPrint(msg_overload);
        goto draw_done;
negative_z:
        nz = a / CONSTRUCTION_CELL - 1;
    }
have_z:

    ndl = 0;
    ndt = 0;
    sx = nx - CONSTRUCTION_SCAN_RADIUS_XZ;
    sy = ny - CONSTRUCTION_SCAN_RADIUS_Y;
    sz = nz - CONSTRUCTION_SCAN_RADIUS_XZ;
    ex = nx + CONSTRUCTION_SCAN_RADIUS_XZ;
    ey = ny + CONSTRUCTION_SCAN_RADIUS_Y;
    ez = nz + CONSTRUCTION_SCAN_RADIUS_XZ;
    SlotMan.max = N_DRAW_SLOTS;
    SlotMan.slot = Slot;
    SlotMan.n = 0;

    for (j = 0; j < N_DRAW_BUCKETS; j++)
        DrawList[j] = 0;

    SetRotMatrix(&GsWSMATRIX);
    *CONSTRUCTION_VISIBILITY_VIEW = ViewInfo;

    {
        WorldType(*world_base)[WORLD_MAP_AXIS_SIZE][WORLD_MAP_AXIS_SIZE];
        int cell_x;
        int cell_y;
        int cell_z;
        int world_y_offset;
        int world_x_offset;
        int visible;

        j = sx;
        for (;;)
        {
            if (j <= ex)
            {
                cell_x = j;
                k = sy;
        scan_y:
            if (k <= ey)
            {
                cell_y = k;
                world_y_offset =
                    (cell_y & WORLD_MAP_AXIS_MASK) * WORLD_MAP_Y_BYTE_STRIDE;
                world_base = WorldMap;
                world_x_offset =
                    (cell_x & WORLD_MAP_AXIS_MASK) * WORLD_MAP_X_BYTE_STRIDE;
                l = sz;
            scan_z:
                if (l <= ez)
                {
                    cell_z = l;
                    do
                    {
                        do
                        {
                            visible = IsVisible(cell_x * CONSTRUCTION_CELL + CONSTRUCTION_CELL_CENTER_OFFSET,
                                                cell_y * CONSTRUCTION_CELL + CONSTRUCTION_CELL_CENTER_OFFSET,
                                                cell_z * CONSTRUCTION_CELL + CONSTRUCTION_CELL_CENTER_OFFSET,
                                                CONSTRUCTION_CELL_VISIBILITY_RADIUS);
                        } while (0);
                    } while (0);
                    if (visible)
                    {
                        cur = ((WorldType *)((cell_z & WORLD_MAP_AXIS_MASK) *
                                                 WORLD_MAP_Z_BYTE_STRIDE +
                                             world_y_offset + world_x_offset +
                                             (u32)world_base))
                                  ->top;
                        for (;;)
                        {
                            if (cur != 0)
                            {
                                if (IsVisible(cur->model->locate.coord.t[0],
                                              cur->model->locate.coord.t[1] + cur->ShiftY,
                                              cur->model->locate.coord.t[2], cur->ModelSize))
                                {
                                    int bucket;
                                    int signed_size;
                                    ObjectSlotType **slot;
                                    OrnamentType *model;

                                    /* IsVisible leaves this object's view-space position behind
                                     * for the depth bucket calculation. */
                                    do
                                    {
                                        do
                                        {
                                            signed_size = cur->ModelSize;
                                            bucket = ((CONSTRUCTION_VISIBILITY_VIEW_SPACE->vz -
                                                       signed_size) >>
                                                      CONSTRUCTION_DEPTH_BUCKET_SHIFT) -
                                                     CONSTRUCTION_DEPTH_BUCKET_BIAS;
                                            plimit = (u16)cur->ModelSize;
                                        } while (0);
                                    } while (0);
                                    if (bucket < 0)
                                        bucket = 0;
                                    slot = (ObjectSlotType **)(bucket * sizeof(*slot) + (u32)DrawList);
                                    model = cur->model;

                                    if (SlotMan.n >= SlotMan.max)
                                        AdtMessageBox(msg_modelslot_overflow);
                                    SlotMan.slot[SlotMan.n].model = model;
                                    SlotMan.slot[SlotMan.n].next = *slot;
                                    SlotMan.slot[SlotMan.n].ModelSize = plimit;
                                    SlotMan.slot[SlotMan.n].ShiftY = 0;
                                    *slot = &SlotMan.slot[SlotMan.n];
                                    ndl++;
                                    SlotMan.n++;
                                }
                                ndt++;
                                cur = cur->next;
                                continue;
                            }
                            break;
                        }
                    }
                    l++;
                    goto scan_z;
                }
                k++;
                goto scan_y;
            }
                j++;
                continue;
            }
            break;
        }
    }
    {
        GsOT ot;
        PACKET *packet_base;

        packet_base = GsGetWorkBase();
        DrawTMDmode = TMD_BANK_FOG;
        ot = *OTablePt;
        ot.org += CONSTRUCTION_LOCAL_OT_OFFSET;

        cur = DrawList[0];
        for (;;)
        {
            if (cur != 0)
            {
                if (cur->model != 0)
                {
                    GsGetLs(&cur->model->locate,
                            (MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
                    GsSetLsMatrix((MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
                    GsSortObject4(&cur->model->object, &ot, 2,
                                  (u_long *)TENCHU_SCRATCHPAD_ADDRESS);
                }
                cur = cur->next;
                continue;
            }
            break;
        }

        j = 1;
        for (;;)
        {
            if (j < N_DRAW_BUCKETS)
            {
                cur = DrawList[j];
                for (;;)
                {
                    if (cur != 0)
                    {
                        if (cur->model != 0)
                        {
                            GsGetLs(&cur->model->locate,
                                    (MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
                            GsSetLsMatrix((MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
                            DrawTMD(&cur->model->object, OTablePt, 0);
                        }
                        if ((u32)(GsGetWorkBase() - packet_base) > 0x6400) /* per-frame construction packet budget */
                            goto overload;
                        cur = cur->next;
                        continue;
                    }
                    break;
                }
                j++;
                continue;
            }
            break;
        }

    draw_done:
        if (GetPad(PAD_CONTROLLER_1) & PADselect)
        {
            FntPrint(str_map);
            FntPrint(fmt_objs_d, ndl, ndt);
            FntPrint(str_newline_2);
            FntPrint(fmt_pk_size, GsGetWorkBase() - packet_base);
        }
    }
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int leFindEnemy(void);
 *     WORLD.C:1099, 31 src lines, frame 88 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       int i
 *     reg   $s6       long px
 *     reg   $s5       long py
 *     reg   $s4       long pz
 *     reg   $s3       int find
 *     reg   $s2       int r
 *     reg   $v1       int rr
 *     stack sp+16     struct SVECTOR pow
 *     stack sp+24     struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

enemy_layout_index leFindEnemy(void)
{
    int i;
    s32 px, py, pz;
    enemy_layout_index find;
    int r;
    int rr;
    int dx, dy, dz;

    find = ENEMY_LAYOUT_NONE;
    r = 2000;
    px = CamState.Owner->model->locate.coord.t[0];
    py = CamState.Owner->model->locate.coord.t[1];
    pz = CamState.Owner->model->locate.coord.t[2];

    i = 0;
    while (1)
    {
        if (i >= MAX_ENEMIES)
            break;
        if (enemy[i].type != CHARACTER_KIND_END)
        {
            dx = enemy[i].x - px;
            dy = enemy[i].y - py;
            dz = enemy[i].z - pz;
            rr = SquareRoot0(dx * dx + dy * dy + dz * dz);
            if (rr < r)
            {
                find = i;
                r = rr;
            }
        }
        i++;
    }

    if (find != ENEMY_LAYOUT_NONE)
    {
        SVECTOR pow = {
            .vx = 0,
            .vy = -100,
            .vz = 0
        };
        VECTOR pos = {
            .vx = enemy[find].x,
            .vy = enemy[find].y,
            .vz = enemy[find].z
        };

        SetExplosion(&pos, &pow);
    }

    return find;
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void leLayoutEnemy(int mode);
 *     WORLD.C:1197, 58 src lines, frame 104 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s5       int mode
 *     reg   $s3       int i
 *     reg   $s1       struct Humanoid * target
 *     stack sp+24     struct VECTOR pos
 *     reg   $s0       struct TraceLine * t
 *     reg   $s0       struct TEnemyLayout * en
 *     reg   $s1       struct Humanoid * human
 *     reg   $s1       struct Humanoid * human
 *     reg   $s0       struct TEnemyLayout * en
 *     reg   $a1       struct TracePoint * tp
 *     reg   $a0       int i
 *     stack sp+40     struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short Humans;
 *     extern enum TSystemFlag SystemFlag;
 *     extern struct TEnemyLayout enemy[30];
 *     extern struct TCameraStatus CamState;
 *     extern long EmergencyNotice;
 *     extern long GameClock;
 * END PSX.SYM */



void leLayoutEnemy(enemy_layout_mode mode)
{
    s32 i;
    Humanoid *target;
    VECTOR tmp;
    VECTOR pos;
    TraceLine *t;
    Humanoid **group;

    reset_effects_();
    group = HumanGroup;
    while (1)
    {
        if (Humans <= 1)
        {
            break;
        }
        target = group[1];
        if ((SystemFlag & SYSFLAG_DEBUGMODE) != 0)
        {
            memset(&pos, 0, sizeof(pos));
            pos.vx = target->model->locate.coord.t[0];
            pos.vy = target->model->locate.coord.t[1] - 1200;
            pos.vz = target->model->locate.coord.t[2];
            tmp = pos;
            SetBleeds(&tmp, 600, 20, 10, 10, COLOR_YELLOW);
        }
        t = target->trace;
        if (t != 0)
        {
            vfree(t->point);
            vfree(t);
        }
        KillHumanoid(target);
    }

    i = 0;
    while (1)
    {
        TEnemyLayout *en;

        if (i >= MAX_ENEMIES)
        {
            break;
        }
        en = &enemy[i];
        if (en->type != CHARACTER_KIND_END)
        {
            Humanoid *human;
            ModelArchiveType *owner_model;

            human = BreedLife(en->type, en->x, en->y, en->z, 0);
            human->model->rotate.vy = en->r;
            owner_model = CamState.Owner->model;
            human->attribute |= ATTR_SUSPEND;
            human->target = &owner_model->locate;
            human->model->object[MODEL_PART_WAIST]->attribute &= ~MODEL_ATTR_COLLIDE;
            if (mode == ENEMY_LAYOUT_GAMEPLAY)
            {
                SetupThinkFunction(human, en->ThinkType);
                if (en->nPath != 0)
                {
                    TracePoint *tp;
                    s32 i;

                    tp = valloc((en->nPath + 1) * sizeof(TracePoint));
                    for (i = 0; i < en->nPath; i++)
                    {
                        tp[i].x = en->path[i].vx;
                        tp[i].z = en->path[i].vz;
                        tp[i].range = 1500;
                        tp[i].pad = 0;
                    }
                    tp[i].pad = TRACE_POINT_END;
                    SetupTraceLine(human, tp);
                }
                if (human->trace != 0)
                {
                    human->attribute |= ATTR_TRACE;
                }
            }
            if ((SystemFlag & SYSFLAG_DEBUGMODE) != 0)
            {
                memset(&pos, 0, sizeof(pos));
                pos.vx = human->model->locate.coord.t[0];
                pos.vy = human->model->locate.coord.t[1] - 1200;
                pos.vz = human->model->locate.coord.t[2];
                tmp = pos;
                SetBleeds(&tmp, 400, 0, 20, 15, COLOR_WHITE);
            }
        }
        i++;
    }
    EmergencyNotice = 0;
    GameClock = 0;
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void leAddPath(int id, long x, long y, long z);
 *     WORLD.C:1303, 17 src lines, frame 56 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int id
 *     param $a1       long x
 *     param $a2       long y
 *     param $a3       long z
 *     stack sp+16     struct VECTOR pos
 *     stack sp+32     struct SVECTOR pow
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */


void leAddPath(enemy_layout_index id, s32 x, s32 y, s32 z)
{
    TEnemyLayout *e;

    if ((u32)id < MAX_ENEMIES)
    {
        e = &enemy[id];
        if (e->nPath < MAX_ENEMY_PATH_POINTS)
        {
            (&e->path[0])[e->nPath].vx = x;
            (&e->path[0])[e->nPath].vy = y;
            (&e->path[0])[e->nPath].vz = z;
            e->nPath++;
            {
                VECTOR pos = {
                    .vx = x,
                    .vy = y,
                    .vz = z
                };
                SVECTOR pow = {
                    .vx = 0,
                    .vy = -100,
                    .vz = 0
                };

                SetExplosion(&pos, &pow);
            }
        }
    }
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void leResetPath(int id);
 *     WORLD.C:1293, 6 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int id
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

void leResetPath(enemy_layout_index id)
{
    if ((u32)id < MAX_ENEMIES)
    {
        enemy[id].nPath = 0;
    }
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void leRestoreEnemyLayout(void *buf);
 *     WORLD.C:1286, 2 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       void * buf
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */


void leRestoreEnemyLayout(void *buf)
{
    memcpy(enemy, buf, sizeof(enemy));
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void lePackEnemyLayout(void *buf, long size);
 *     WORLD.C:1275, 7 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       void * buf
 *     param $a1       long size
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

extern char fmt_enemy_storing_size_too[]; /* enemy storing size too small %d/%d */

void lePackEnemyLayout(void *buf, long size)
{
    if (size < sizeof(enemy))
    {
        AdtMessageBox(fmt_enemy_storing_size_too, size, sizeof(enemy));
    }
    else
    {
        memcpy(buf, enemy, sizeof(enemy));
    }
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int leRemoveEnemy(void);
 *     WORLD.C:1259, 12 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

int leRemoveEnemy(void)
{
    enemy_layout_index idx;

    idx = leFindEnemy();
    if (idx == ENEMY_LAYOUT_NONE)
    {
        return 0;
    }
    enemy[idx].type = CHARACTER_KIND_END;
    leLayoutEnemy(ENEMY_LAYOUT_EDIT);
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DestroyTraceLine(struct TraceLine *t);
 *     WORLD.C:1186, 7 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct TraceLine * t
 * END PSX.SYM */

static void DestroyTraceLine(TraceLine *t)
{
    if (t != 0)
    {
        vfree(t->point);
        vfree(t);
    }
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int leSetEnemy(int type, short think, long x, long y, long z, int r);
 *     WORLD.C:1134, 22 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int type
 *     param $a1       short think
 *     param $a2       long x
 *     param $a3       long y
 *     param stack+16  long z
 *     param stack+20  int r
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

enemy_layout_index leSetEnemy(s32 type, TThinkType think, s32 x, s32 y,
                              s32 z, s16 r)
{
    enemy_layout_index idx;
    enemy_layout_index result;
    s32 offset;
    TEnemyLayout *e;

    idx = 0;
    for (;;)
    {
        if (enemy[idx].type != CHARACTER_KIND_END)
        {
            idx++;
            if (idx < MAX_ENEMIES)
            {
                continue;
            }
            result = ENEMY_LAYOUT_NONE;
        }
        else
        {
            result = idx;
        }
        break;
    }
    if (result == ENEMY_LAYOUT_NONE)
        return ENEMY_LAYOUT_NONE;
    offset = result * sizeof(*e);
    e = (TEnemyLayout *)(offset + (s32)enemy);
    e->type = (s16)type;
    e->ThinkType = think;
    e->nPath = 0;
    e->x = x;
    e->y = y;
    e->z = z;
    e->r = r;
    return result;
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void leClearLayout(void);
 *     WORLD.C:1065, 9 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

void leClearLayout(void)
{
    character_kind dead;
    s32 i;

    dead = CHARACTER_KIND_END;
    for (i = MAX_ENEMIES - 1; i >= 0; i--)
    {
        enemy[i].type = dead;
    }
    leLayoutEnemy(ENEMY_LAYOUT_EDIT);
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void leResetEnemyLayout(void);
 *     WORLD.C:1054, 7 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

void leResetEnemyLayout(void)
{
    character_kind dead;
    s32 i;

    dead = CHARACTER_KIND_END;
    for (i = MAX_ENEMIES - 1; i >= 0; i--)
    {
        enemy[i].type = dead;
    }
}


extern char msg_load_layout_error[]; /* load layout error */
extern u8 *LayoutNames[N_STAGE_LAYOUTS];

void load_layout(s32 index)
{
    LayoutSaveData *layout;
    u8 *names[N_STAGE_LAYOUTS];

    __builtin_memcpy(names, LayoutNames, sizeof(names));
    layout = LoadSI(SAVE_STORAGE_DISK, names[index]);
    if (layout == 0)
    {
        AdtMessageBox(msg_load_layout_error);
    }
    else
    {
        leRestoreEnemyLayout(layout->enemies);
        RestoreItemLayout(layout->items);
        vfree(layout);
    }
    leLayoutEnemy(ENEMY_LAYOUT_GAMEPLAY);
}


void load_save_slot_(enum save_storage storage, u8 *name)
{
    LayoutSaveData *layout;

    layout = LoadSI(storage & 0xFF, name);
    if (layout == 0)
    {
        AdtMessageBox(msg_load_layout_error);
    }
    else
    {
        leRestoreEnemyLayout(layout->enemies);
        RestoreItemLayout(layout->items);
        vfree(layout);
    }
}


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeOrnamentArchive(struct OrnamentArchiveType *mad);
 *     WORLD.C:319, 11 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct OrnamentArchiveType * mad
 * END PSX.SYM */

void DisposeOrnamentArchive(OrnamentArchiveType *mad)
{
    s32 i;

    if (mad != 0)
    {
        for (i = 0; i < mad->n; i++)
        {
            DisposeOrnament(mad->object[i]);
        }
        vfree(mad->object);
        vfree(mad->data);
        vfree(mad);
    }
}
