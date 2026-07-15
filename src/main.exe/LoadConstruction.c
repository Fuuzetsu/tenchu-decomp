#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short LoadConstruction(unsigned long *data);
 *     WORLD.C:436, 191 src lines, frame 416 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * STATUS: NON_MATCHING — complete semantic C draft. It has the target's
 * 0x1a0 frame and exact stack layout, including the filename buffer, both
 * PARAM_ITEM_STAY records, and upper scalar locals. The draft is 2744 bytes,
 * 20 bytes longer than the 2724-byte split target; remaining work is codegen
 * alignment rather than missing behavior. Build it with
 * `NON_MATCHING=LoadConstruction ./Build`. On a full match, delete the guards
 * and the stub-only _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", LoadConstruction);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", switchD_8003ae64__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", switchD_8003ae64__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", reinitialise_models_and_stuff___override__prt_8003ae78_f3434af);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", switchD_8003ae64__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", reinitialise_models_and_stuff___override__prt_8003aef4_f3434af);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", switchD_8003ae64__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", switchD_8003ae64__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", switchD_8003ae64__caseD_b);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", switchD_8003ae64__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/LoadConstruction", switchD_8003ae64__caseD_1);

/* jump-table pool @ 0x8001219c (12 words; tables at 0x8001219c) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 LoadConstruction_jtbl[12] = {
    0x8003AE6C, 0x8003B25C, 0x8003AF1C, 0x8003B17C,
    0x8003B1E8, 0x8003AEE8, 0x8003B25C, 0x8003B25C,
    0x8003B25C, 0x8003B25C, 0x8003B25C, 0x8003B1A4,
};

#else /* NON_MATCHING */

struct OrnamentType
{
    GsCOORDINATE2 locate;
    GsDOBJ2 object;
};

typedef struct
{
    GsCOORDINATE2 locate;
    SVECTOR rotate;
    s16 id;
    s16 attribute;
    s16 n;
    OrnamentType **object;
    u32 *data;
} OrnamentArchiveType;

typedef struct tag_ObjectSlotType ObjectSlotType;
struct tag_ObjectSlotType
{
    ObjectSlotType *next;
    OrnamentType *model;
    s16 ModelSize;
    s16 ShiftY;
};

typedef struct
{
    ObjectSlotType *slot;
    s32 n;
    s32 max;
} ObjectSlotManager;

typedef struct
{
    ObjectSlotType *top;
} WorldMapCell;

typedef struct
{
    s16 type;
    s16 ObjectID;
    union
    {
        char name[12];
        OrnamentType *model;
        s32 misc[3];
    } data;
    s32 x;
    s32 y;
    s32 z;
    s32 n;
} WorldDataType;

typedef struct
{
    s16 parent;
    s16 id;
    s16 x;
    s16 y;
    s16 z;
    s16 pad;
    s32 offset;
} ParentingType;

typedef struct
{
    int ObjectID;
    u32 *MapModel;
    WorldDataType *wlddt;
    long n;
    ParentingType *ix;
    int msize;
} LoadConstructionStack;

typedef struct
{
    unsigned char name[256];
    SVECTOR center;
    PARAM_ITEM_STAY param;
    u32 pad0;
    PARAM_ITEM_STAY tmp;
    u32 pad1;
    int size;
    LoadConstructionStack stack;
} LoadConstructionScratch;

extern WorldMapCell WorldMap[8][8][8];
extern u32 *GlobalAreaMap;
extern u8 *ImagePath;
extern ObjectSlotManager ModelSlot;
extern s32 StageID;
extern ModelType World;
extern OrnamentArchiveType *D_80097A70;
extern OrnamentArchiveType *D_80097A74;
extern u32 *D_800976E8;

extern char D_800120C4[];
extern char D_800120D8[];
extern char D_800120F0[];
extern char D_80012108[];
extern char D_80012114[];
extern char D_80012120[];
extern char D_80097A78[];
extern char D_80097A80[];
extern char D_80097A88[];
extern char D_80097A90[];

extern void SystemOut(char *message);
extern u32 vsize(void *data);
extern void DisposeAreaMap(void *area);
extern void ResetAllMisc(void);
extern void ClearItemLayout(void);
extern void DisposeOrnament(OrnamentType *model);
extern void vfree(void *ptr);
extern void LoadTIMpackAndFree(u32 *data);
extern void *valloc(u32 size);
extern OrnamentArchiveType *LoadOrnamentArchive(u32 *data, ModelType *parent);
extern u32 *LoadAreaMap(u32 *data);
extern u32 *handle_balmer_acm_(u32 *data);
extern void UpdateOrnament(OrnamentType *model, s16 ry);
extern void GetCenterAndSize(u32 *tmd, SVECTOR *center, int *size);
extern OrnamentType *CreateCloneOrnament(OrnamentType *model);
extern Humanoid *BreedLife(s16 type, s32 x, s32 y, s32 z, s32 r);
extern void AddMisc(s32 type, s32 x, s32 y, s32 z,
                    s32 a, s32 b, s32 c);
extern void jt_init4(void);

short LoadConstruction(u32 *data)
{
    OrnamentType *model;
    long x;
    long y;
    long z;
    long i;
    int nModel;
    OrnamentArchiveType *mad;
    ObjectSlotType **slot;
    short shifty;
    WorldDataType *cur;
    ObjectSlotManager *slotman;
    LoadConstructionScratch scratch;

    scratch.stack.ObjectID = 0;
    scratch.stack.wlddt = (WorldDataType *)data;
    if (scratch.stack.wlddt == 0)
        SystemOut(D_800120D8);
    memset(WorldMap, 0, sizeof(WorldMap));

    nModel = 0;
    scratch.stack.n = vsize(data) >> 5;
    for (i = 0; i < scratch.stack.n; i++)
    {
        if (scratch.stack.wlddt[i].type == 2)
            nModel++;
    }

    DisposeAreaMap(GlobalAreaMap);
    GlobalAreaMap = 0;
    ResetAllMisc();
    ClearItemLayout();

    mad = D_80097A70;
    if (mad != 0)
    {
        for (i = 0; i < mad->n; i++)
            DisposeOrnament(mad->object[i]);
        vfree(mad->object);
        vfree(mad->data);
        vfree(mad);
    }

    mad = D_80097A74;
    if (mad != 0)
    {
        for (i = 0; i < mad->n; i++)
            DisposeOrnament(mad->object[i]);
        vfree(mad->object);
        vfree(mad->data);
        vfree(mad);
    }

    LoadTIMpackAndFree((u32 *)PathFileRead((char *)ImagePath, D_80097A78));
    LoadTIMpackAndFree((u32 *)PathFileRead(D_800120F0, D_80012108));

    scratch.stack.MapModel = (u32 *)valloc(0x6B800);
    D_80097A74 = LoadOrnamentArchive(
        (u32 *)PathFileRead((char *)ImagePath, D_80012114), &World);

    slotman = &ModelSlot;
    if (0 < slotman->max)
    {
        int offset;

        offset = 0;
        for (i = 0; i < slotman->n; i++)
        {
            model = *(OrnamentType **)((u8 *)&slotman->slot->model + offset);
            if (((u32)model & 0xFF000000) == 0x80000000)
                DisposeOrnament(model);
            offset += sizeof(ObjectSlotType);
        }
        vfree(slotman->slot);
    }

    slotman->max = nModel + 500;
    slotman->slot = (ObjectSlotType *)valloc(slotman->max * sizeof(ObjectSlotType));
    slotman->n = 0;

    i = 0;
    cur = scratch.stack.wlddt;
    while (i < scratch.stack.n)
    {
        switch (cur->type)
        {
    case 0:
        sprintf((char *)scratch.name, D_80097A80, cur->data.name);
        DisposeAreaMap(GlobalAreaMap);
        GlobalAreaMap = LoadAreaMap(
            (u32 *)PathFileRead((char *)ImagePath, (char *)scratch.name));
        if (StageID == 4)
        {
            D_800976E8 = handle_balmer_acm_(
                (u32 *)PathFileRead((char *)ImagePath, D_80012120));
        }
        break;

    case 5:
        sprintf((char *)scratch.name, D_80097A88, cur->data.name);
        LoadTIMAndFree((u_long *)PathFileRead(D_800120F0, (char *)scratch.name));
        break;

    case 2:
        if (cur->data.name[0] == 0)
            model = CreateCloneOrnament(scratch.stack.wlddt[cur->ObjectID].data.model);
        else
        {
            model = D_80097A74->object[scratch.stack.ObjectID];
            scratch.stack.ObjectID++;
            cur->data.model = model;
        }

        model->locate.coord.t[0] = cur->x;
        model->locate.coord.t[1] = cur->y;
        model->locate.coord.t[2] = cur->z;
        UpdateOrnament(model, cur->n);

        if (cur->x < 0)
            x = cur->x / 16000 - 1;
        else
            x = cur->x / 16000;
        if (cur->y < 0)
            y = cur->y / 16000 - 1;
        else
            y = cur->y / 16000;
        if (cur->z < 0)
            z = cur->z / 16000 - 1;
        else
            z = cur->z / 16000;

        GetCenterAndSize(model->object.tmd, &scratch.center, &scratch.size);
        shifty = scratch.center.vy;
        scratch.stack.msize = scratch.size / 2;
        slot = &WorldMap[x & 7][y & 7][z & 7].top;
        if (ModelSlot.n >= ModelSlot.max)
            AdtMessageBox(D_800120C4);
        ModelSlot.slot[ModelSlot.n].model = model;
        ModelSlot.slot[ModelSlot.n].next = *slot;
        ModelSlot.slot[ModelSlot.n].ModelSize = scratch.stack.msize;
        ModelSlot.slot[ModelSlot.n].ShiftY = shifty;
        *slot = &ModelSlot.slot[ModelSlot.n];
        ModelSlot.n++;
        break;

    case 3:
        BreedLife(cur->ObjectID, cur->x, cur->y, cur->z, cur->n);
        cur++;
        i++;
        continue;

    case 11:
        AddMisc(cur->data.misc[0], cur->data.misc[1], cur->data.misc[2],
                cur->x, cur->y, cur->z, cur->n);
        cur++;
        i++;
        continue;

    case 4:
        memset(&scratch.tmp, 0, sizeof(scratch.tmp));
        scratch.tmp.type = cur->ObjectID;
        scratch.tmp.locate.vx = cur->x;
        scratch.tmp.locate.vy = cur->y;
        scratch.tmp.locate.vz = cur->z;
        scratch.param = scratch.tmp;
        ReqItemStay(&scratch.param);
        break;
        }

        cur++;
        i++;
    }

    sprintf((char *)scratch.name, D_80097A90);
    vfree(scratch.stack.MapModel);
    scratch.stack.MapModel = (u32 *)PathFileRead((char *)ImagePath, (char *)scratch.name);
    scratch.stack.ix = (ParentingType *)(scratch.stack.MapModel + 2);
    D_80097A70 = LoadOrnamentArchive(scratch.stack.MapModel, &World);

    {
        int objectOffset;

        objectOffset = 0;
        for (i = 0; i < D_80097A70->n; i++)
        {
            OrnamentType *entry;

            entry = *(OrnamentType **)((u8 *)D_80097A70->object + objectOffset);
            entry->locate.coord.t[0] *= 10;
            entry = *(OrnamentType **)((u8 *)D_80097A70->object + objectOffset);
            entry->locate.coord.t[1] *= 10;
            entry = *(OrnamentType **)((u8 *)D_80097A70->object + objectOffset);
            entry->locate.coord.t[2] *= 10;
            scratch.stack.msize = scratch.stack.ix[i].parent * -14;

            entry = *(OrnamentType **)((u8 *)D_80097A70->object + objectOffset);
            if (entry->locate.coord.t[0] < 0)
                x = entry->locate.coord.t[0] / 16000 - 1;
            else
                x = entry->locate.coord.t[0] / 16000;
            entry = *(OrnamentType **)((u8 *)D_80097A70->object + objectOffset);
            if (entry->locate.coord.t[1] < 0)
                y = entry->locate.coord.t[1] / 16000 - 1;
            else
                y = entry->locate.coord.t[1] / 16000;
            entry = *(OrnamentType **)((u8 *)D_80097A70->object + objectOffset);
            if (entry->locate.coord.t[2] < 0)
                z = entry->locate.coord.t[2] / 16000 - 1;
            else
                z = entry->locate.coord.t[2] / 16000;

            entry = *(OrnamentType **)((u8 *)D_80097A70->object + objectOffset);
            entry->object.attribute |= 0x400;
            UpdateOrnament(entry, 0);
            slot = &WorldMap[x & 7][y & 7][z & 7].top;
            model = *(OrnamentType **)((u8 *)D_80097A70->object + objectOffset);
            if (ModelSlot.n >= ModelSlot.max)
                AdtMessageBox(D_800120C4);
            ModelSlot.slot[ModelSlot.n].model = model;
            ModelSlot.slot[ModelSlot.n].next = *slot;
            ModelSlot.slot[ModelSlot.n].ModelSize = scratch.stack.msize;
            ModelSlot.slot[ModelSlot.n].ShiftY = 0;
            *slot = &ModelSlot.slot[ModelSlot.n];
            ModelSlot.n++;
            objectOffset += sizeof(OrnamentType *);
        }
    }

    vfree(data);
    jt_init4();
    return -1;
}

#endif /* NON_MATCHING */
