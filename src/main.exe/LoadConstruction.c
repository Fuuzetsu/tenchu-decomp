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
 * PARAM_ITEM_STAY records, and upper scalar locals. Demo SLD evidence restored
 * the indexed construction-record and ornament loops. Target-evidenced
 * top-tested loop forms, a register-resident first-pass model size, and the
 * final pass's early signed parent carrier reproduce the target's full
 * saved-register set and exact 2724-byte/681-instruction extent. An explicit
 * initial disposal test with hoisted pointer masks now matches that traversal
 * exactly; preserving the shared nModel/WorldMap-top source identity and the
 * case-2 and final-preparation allocation boundaries also recover the target's
 * first-pass slot and model registers.
 *
 * The linked residual is now 643 differing bytes (fuzzy 86.34), down from 666.
 * What closed this pass: BOTH WorldMap slot addresses (case-2 nModel, final-pass
 * slot) are spelled as explicit integer offset arithmetic
 * `(z<<2) + ((x<<8) + (y<<5)) + (int)WorldMap` instead of `&WorldMap[x][y][z].top`.
 * The ARRAY_REF form folded the base early and evaluated z last; the target
 * computes the pure offset (z-term first, x/y grouped) and adds the base symbol
 * as a separate trailing integer term (rule "arr[idx] expands base-first"),
 * matching the target's evaluation tree exactly (only the x/y register homes
 * still differ). A guided-autorules do{}while(0) fence on the case-2 clone load
 * (L~401) closed 2 more bytes.
 *
 * Remaining residual is entirely parameter-home scheduling + regalloc rotation,
 * not missing behavior:
 *   - x/y are one callee-saved register LOW vs retail (ours x=$s4/y=$s3, retail
 *     x=$s5/y=$s4; z=$s0, model=$s2, nModel=$s3 all match). Root: ours coalesces
 *     the disjoint {y, nModel} into $s3 while retail keeps them apart. The demo
 *     SLD ALSO coalesces (x=$s4,y=$s3), so this is a retail-only non-coalescing
 *     divergence. Splitting the case-2 WorldMap addr into its own short-lived
 *     `slot` local (the demo's separate `slot` var) forces the split but REGRESSES
 *     +5 (rotates the first-pass), confirming the shared nModel identity is
 *     load-bearing. Raising nModel priority to out-rank y needs ~64 refs
 *     (unreachable); y and nModel are genuinely disjoint so no conflict forces
 *     the split. Not reachable by the C-level levers tried.
 *   - The `data` param-home store `sw a0,0x1A0(sp)` schedules LATE (after the
 *     callee saves) vs the target's position 2. `data` is the lowest-priority
 *     pseudo (reg 80, 47 call-crossings / few refs) so reload spills it as a body
 *     insn placed after the RTL register saves; the target homes it first.
 *     `u32 *volatile data` regressed +4; statement reorder of the vsize/i=nModel
 *     pair regressed +2.
 * Build it with `NON_MATCHING=LoadConstruction ./Build`. On a full match, delete
 * the guards and the stub-only _jtbl array.
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
    WorldDataType *wlddt;
    long x;
    long y;
    long z;
    int nModel;
    long i;
    short shifty;
    LoadConstructionScratch scratch;

    scratch.stack.ObjectID = 0;
    scratch.stack.wlddt = (WorldDataType *)data;
    if (scratch.stack.wlddt == 0)
        SystemOut(D_800120D8);
    memset(WorldMap, 0, sizeof(WorldMap));

    {
        WorldDataType *scan;

        nModel = 0;
        scratch.stack.n = vsize(data) >> 5;
        i = nModel;
        if (scratch.stack.n != 0)
        {
            scan = (WorldDataType *)data;
            do
            {
                if (scan->type == 2)
                    nModel++;
                i++;
                scan++;
            } while (i < *(volatile long *)&scratch.stack.n);
        }
    }

    DisposeAreaMap(GlobalAreaMap);
    GlobalAreaMap = 0;
    ResetAllMisc();
    ClearItemLayout();

    {
        OrnamentArchiveType *mad;
        int i;

        mad = D_80097A70;
        if (mad != 0)
        {
            for (i = 0; i < mad->n; i++)
                DisposeOrnament(mad->object[i]);
            vfree(mad->object);
            vfree(mad->data);
            vfree(mad);
        }
    }

    {
        OrnamentArchiveType *mad;
        int i;

        mad = D_80097A74;
        if (mad != 0)
        {
            for (i = 0; i < mad->n; i++)
                DisposeOrnament(mad->object[i]);
            vfree(mad->object);
            vfree(mad->data);
            vfree(mad);
        }
    }

    LoadTIMpackAndFree((u32 *)PathFileRead((char *)ImagePath, D_80097A78));
    LoadTIMpackAndFree((u32 *)PathFileRead(D_800120F0, D_80012108));

    scratch.stack.MapModel = (u32 *)valloc(0x6B800);
    D_80097A74 = LoadOrnamentArchive(
        (u32 *)PathFileRead((char *)ImagePath, D_80012114), &World);

    nModel += 500;
    {
        OrnamentType *disposeModel;
        int i;
        int offset;
        ObjectSlotManager *slotman;

        slotman = &ModelSlot;
        if (0 < slotman->max)
        {
            u32 mask;
            u32 base;

            i = 0;
            if (i < slotman->n)
            {
                mask = 0xFF000000;
                base = 0x80000000;
                offset = i;
                for (; i < slotman->n; i++)
                {
                    disposeModel = *(OrnamentType **)((u8 *)&slotman->slot->model + offset);
                    if (((u32)disposeModel & mask) == base)
                        DisposeOrnament(disposeModel);
                    offset += sizeof(ObjectSlotType);
                }
            }
            vfree(slotman->slot);
        }

        slotman->max = nModel;
        slotman->slot = (ObjectSlotType *)valloc(nModel * sizeof(ObjectSlotType));
        slotman->n = 0;
    }

    {
        int msize;
        ObjectSlotManager *slotman;

        wlddt = scratch.stack.wlddt;
        i = 0;
        while (1)
        {
            if (!(i < scratch.stack.n))
                break;
            switch (wlddt[i].type)
            {
    case 0:
        sprintf((char *)scratch.name, D_80097A80, wlddt[i].data.name);
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
        sprintf((char *)scratch.name, D_80097A88, wlddt[i].data.name);
        LoadTIMAndFree((u_long *)PathFileRead(D_800120F0, (char *)scratch.name));
        break;

    case 2:
        do
        {
            do
            {
                if (wlddt[i].data.name[0] != 0)
                {
                    do {
                      model = D_80097A74->object[scratch.stack.ObjectID];
                    } while (0);
                    scratch.stack.ObjectID++;
                    wlddt[i].data.model = model;
                }
                else
                    model = CreateCloneOrnament(
                        scratch.stack.wlddt[wlddt[i].ObjectID].data.model);

                model->locate.coord.t[0] = wlddt[i].x;
                model->locate.coord.t[1] = wlddt[i].y;
                model->locate.coord.t[2] = wlddt[i].z;
                UpdateOrnament(model, wlddt[i].n);
            } while (0);
        } while (0);

        {
            long a = wlddt[i].x;

            if (a >= 0)
                x = a / 16000;
            else
                x = a / 16000 - 1;
        }
        x &= 7;
        {
            long a = wlddt[i].y;

            if (a >= 0)
                y = a / 16000;
            else
                y = a / 16000 - 1;
        }
        y &= 7;
        {
            long a = wlddt[i].z;

            if (a >= 0)
                z = a / 16000;
            else
                z = a / 16000 - 1;
        }
        z &= 7;

        GetCenterAndSize(model->object.tmd, &scratch.center, &scratch.size);
        nModel = (z << 2) + ((x << 8) + (y << 5)) + (int)WorldMap;
        slotman = &ModelSlot;
        shifty = scratch.center.vy;
        msize = scratch.size / 2;
        if (slotman->n >= slotman->max)
            AdtMessageBox(D_800120C4);
        slotman->slot[slotman->n].model = model;
        slotman->slot[slotman->n].next = *(ObjectSlotType **)nModel;
        slotman->slot[slotman->n].ModelSize = msize;
        slotman->slot[slotman->n].ShiftY = shifty;
        *(ObjectSlotType **)nModel = &slotman->slot[slotman->n];
        slotman->n++;
        break;

    case 3:
        BreedLife(wlddt[i].ObjectID, wlddt[i].x, wlddt[i].y,
                  wlddt[i].z, wlddt[i].n);
        break;

    case 11:
        AddMisc(wlddt[i].data.misc[0], wlddt[i].data.misc[1],
                wlddt[i].data.misc[2], wlddt[i].x, wlddt[i].y,
                wlddt[i].z, wlddt[i].n);
        break;

    case 4:
        memset(&scratch.tmp, 0, sizeof(scratch.tmp));
        scratch.tmp.type = wlddt[i].ObjectID;
        scratch.tmp.locate.vx = wlddt[i].x;
        scratch.tmp.locate.vy = wlddt[i].y;
        scratch.tmp.locate.vz = wlddt[i].z;
        scratch.param = scratch.tmp;
        ReqItemStay(&scratch.param);
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

        sprintf((char *)scratch.name, D_80097A90);
        vfree(scratch.stack.MapModel);
        do {
        } while (0);
        i = 0;
        scratch.stack.MapModel =
            (u32 *)PathFileRead((char *)ImagePath, (char *)scratch.name);
        scratch.stack.ix = (ParentingType *)(scratch.stack.MapModel + 2);
        D_80097A70 = LoadOrnamentArchive(scratch.stack.MapModel, &World);

        while (1)
        {
            if (!(i < D_80097A70->n))
                break;
            parent = scratch.stack.ix[i].parent;
            D_80097A70->object[i]->locate.coord.t[0] *= 10;
            D_80097A70->object[i]->locate.coord.t[1] *= 10;
            D_80097A70->object[i]->locate.coord.t[2] *= 10;

            {
                long a = D_80097A70->object[i]->locate.coord.t[0];

                scratch.stack.msize =
                    (parent - parent * 8) * 2;
                if (a >= 0)
                    x = a / 16000;
                else
                    x = a / 16000 - 1;
            }
            x &= 7;
            {
                long a = D_80097A70->object[i]->locate.coord.t[1];

                if (a >= 0)
                    y = a / 16000;
                else
                    y = a / 16000 - 1;
            }
            y &= 7;
            {
                long a = D_80097A70->object[i]->locate.coord.t[2];

                if (a >= 0)
                    z = a / 16000;
                else
                    z = a / 16000 - 1;
            }
            z &= 7;

            D_80097A70->object[i]->object.attribute |= 0x400;
            UpdateOrnament(D_80097A70->object[i], 0);
            slot = (ObjectSlotType **)((z << 2) + ((x << 8) + (y << 5)) + (int)WorldMap);
            slotman = &ModelSlot;
            model = D_80097A70->object[i];
            if (slotman->n >= slotman->max)
                AdtMessageBox(D_800120C4);
            slotman->slot[slotman->n].model = model;
            slotman->slot[slotman->n].next = *slot;
            slotman->slot[slotman->n].ModelSize = scratch.stack.msize;
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

#endif /* NON_MATCHING */
