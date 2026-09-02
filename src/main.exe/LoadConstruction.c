#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"
#include "misc.h"
#include "tmdfile.h"
#include "vmemory.h"

/* Map a world coordinate to its WorldMap cell index (floor division by the
 * CONSTRUCTION_CELL-unit cell, wrapped to one axis). Repeated per axis at
 * both construction passes; macro is reconstruction shorthand (expands to
 * the identical text). The copy interleaved with the msize computation stays
 * open-coded. */
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

/* Free an ornament archive: every ornament, then the object table, the
 * model data, and the archive record itself. Retail repeats the block
 * for the mission archive and the shared object archive; the macro is
 * reconstruction shorthand for that copy-paste (expands to the
 * identical text). */
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

/*
 * LoadConstruction (0x8003ab60) — rebuild the stage's construction state:
 * dispose the previous maps/models, load the shared and stage archives,
 * dispatch the 32-byte construction records, and bucket their ornaments in
 * WorldMap for rendering.
 *
 * STATUS: MATCH (2724 bytes, split jump-table function).
 *
 * Architecture notes (recovered from the target's own codegen):
 *  - ObjectID/MapModel/wlddt/n (fn scope, declaration order) and ix/msize
 *    (final block) are plain locals whose pseudos lose allocation and spill;
 *    reload assigns slots 0x15C..0x170 in pseudo-number order and emits the
 *    rotating t0-t3 reloads, the lhu msize narrow reload, and the delay-slot
 *    spill stores. data reloads from its arg-home slot 0x1A0.
 *  - `MapModelSize` is WORLD.C's original allocation constant. Retail lowers
 *    it from the demo's 0x70800 bytes to 0x6B800 at the same allocation site.
 *  - name/center/param/tmp/size are five separate stack locals: gcc 2.8.1
 *    rounds each BLKmode slot up to 8 bytes, which yields retail's pads
 *    (param@0x128+24, tmp@0x140+24, size@0x158) with no explicit padding.
 *  - The counting loop indexes ((WorldDataType *)data)[i] directly (no
 *    walker variable): loop.c strength-reduces it to the v1 giv, emitting
 *    the hoisted li 2 before the giv init (target preheader order), and the
 *    giv advance lands in the loop branch delay slot.
 *  - Each of the six /CONSTRUCTION_CELL divisions is `long a, q; if (a >= 0) q = a/CONSTRUCTION_CELL;
 *    else q = a/CONSTRUCTION_CELL - 1; x = q & 7;` — a in a0, quotient in the v0/v1
 *    temps, single andi def into the callee-saved home.
 *  - Both WorldMap cell pointers use the PSX.SYM-recorded `slot` local and
 *    are built in two steps: the typed pointer first carries the derived byte
 *    offset, then receives the WorldMap base. Combining the steps or using a
 *    direct array address changes the slot/msize register allocation. This
 *    keeps the actual list accesses typed without reusing the unrelated
 *    nModel count as an address.
 */

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

extern void DisposeAreaMap(AreaMapType *area);
extern void ResetAllMisc(void);
extern void ClearItemLayout(void);
extern void DisposeOrnament(OrnamentType *model);
extern void vfree(void *ptr);
extern void LoadTIMpackAndFree(u_long *data);
extern void *valloc(u32 size);
extern OrnamentArchiveType *LoadOrnamentArchive(u_long *data, ModelType *parent);
extern AreaMapType *LoadAreaMap(AreaMapType *data);
extern AreaMapType *load_balma_area_map_(AreaMapType *data);
extern void UpdateOrnament(OrnamentType *model, s16 ry);
extern void GetCenterAndSize(TmdObjectRecord *tmd, SVECTOR *center, int *size);
extern OrnamentType *CreateCloneOrnament(OrnamentType *model);
extern void jt_init4(void);

short LoadConstruction(u_long *data)
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
    PARAM_ITEM_STAY tmp;
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
            if (((WorldDataType *)data)[i].mode == WLD_RECORD_OBJECT)
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
                memset(&tmp, 0, sizeof(tmp));
                tmp.type = wlddt[i].id;
                tmp.locate.vx = wlddt[i].real.placement.transform.x;
                tmp.locate.vy = wlddt[i].real.placement.transform.y;
                tmp.locate.vz = wlddt[i].real.placement.transform.z;
                param = tmp;
                ReqItemStay(&param);
                break;
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
