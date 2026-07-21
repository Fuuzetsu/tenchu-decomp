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
 * STATUS: MATCH (2724 bytes, split jump-table function).
 *
 * Architecture notes (recovered from the target's own codegen):
 *  - ObjectID/MapModel/wlddt/n (fn scope, declaration order) and ix/msize
 *    (final block) are plain locals whose pseudos lose allocation and spill;
 *    reload assigns slots 0x15C..0x170 in pseudo-number order and emits the
 *    rotating t0-t3 reloads, the lhu msize narrow reload, and the delay-slot
 *    spill stores. data reloads from its arg-home slot 0x1A0.
 *  - name/center/param/tmp/size are five separate stack locals: gcc 2.8.1
 *    rounds each BLKmode slot up to 8 bytes, which yields retail's pads
 *    (param@0x128+24, tmp@0x140+24, size@0x158) with no explicit padding.
 *  - The counting loop indexes ((WorldDataType *)data)[i] directly (no
 *    walker variable): loop.c strength-reduces it to the v1 giv, emitting
 *    the hoisted li 2 before the giv init (target preheader order), and the
 *    giv advance lands in the loop branch delay slot.
 *  - Each of the six /16000 divisions is `long a, q; if (a >= 0) q = a/16000;
 *    else q = a/16000 - 1; x = q & 7;` — a in a0, quotient in the v0/v1
 *    temps, single andi def into the callee-saved home.
 *  - Both WorldMap slot addresses split into offset-then-+base statements
 *    (`nModel = (z<<2)+((x<<8)+(y<<5)); nModel += (int)WorldMap;`), which
 *    puts the partial sum in the carrier register and orders the final-pass
 *    slot allocno above the strength-reduction giv so the whole retail
 *    register cascade (slot=s2, giv=s3, y=s4, x=s5, magic=s6, i=s7, hi=fp,
 *    msize spilled) falls out at natural priorities, fence-free.
 */


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

extern WorldType WorldMap[8][8][8];
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
    int ObjectID;
    u32 *MapModel;
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
        SystemOut(D_800120D8);
    memset(WorldMap, 0, sizeof(WorldMap));

    nModel = 0;
    n = vsize(data) >> 5;
    i = nModel;
    if (n != 0)
    {
        do
        {
            if (((WorldDataType *)data)[i].type == 2)
                nModel++;
            i++;
        } while (i < n);
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

    MapModel = (u32 *)valloc(0x6B800);
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

        i = 0;
        while (1)
        {
            if (!(i < n))
                break;
            switch (wlddt[i].type)
            {
    case 0:
        sprintf((char *)name, D_80097A80, wlddt[i].data.name);
        DisposeAreaMap(GlobalAreaMap);
        GlobalAreaMap = LoadAreaMap(
            (u32 *)PathFileRead((char *)ImagePath, (char *)name));
        if (StageID == 4)
        {
            D_800976E8 = handle_balmer_acm_(
                (u32 *)PathFileRead((char *)ImagePath, D_80012120));
        }
        break;

    case 5:
        sprintf((char *)name, D_80097A88, wlddt[i].data.name);
        LoadTIMAndFree((u_long *)PathFileRead(D_800120F0, (char *)name));
        break;

    case 2:
        if (wlddt[i].data.name[0] != 0)
        {
            model = D_80097A74->object[ObjectID];
            ObjectID++;
            wlddt[i].data.model = model;
        }
        else
            model = CreateCloneOrnament(
                wlddt[wlddt[i].ObjectID].data.model);

        model->locate.coord.t[0] = wlddt[i].x;
        model->locate.coord.t[1] = wlddt[i].y;
        model->locate.coord.t[2] = wlddt[i].z;
        UpdateOrnament(model, wlddt[i].n);

        {
            long a = wlddt[i].x;
            long q;

            if (a >= 0)
                q = a / 16000;
            else
                q = a / 16000 - 1;
            x = q & 7;
        }
        {
            long a = wlddt[i].y;
            long q;

            if (a >= 0)
                q = a / 16000;
            else
                q = a / 16000 - 1;
            y = q & 7;
        }
        {
            long a = wlddt[i].z;
            long q;

            if (a >= 0)
                q = a / 16000;
            else
                q = a / 16000 - 1;
            z = q & 7;
        }

        GetCenterAndSize(model->object.tmd, &center, &size);
        nModel = (z << 2) + ((x << 8) + (y << 5));
        nModel = nModel + (int)WorldMap;
        slotman = &ModelSlot;
        shifty = center.vy;
        msize = size / 2;
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
        memset(&tmp, 0, sizeof(tmp));
        tmp.type = wlddt[i].ObjectID;
        tmp.locate.vx = wlddt[i].x;
        tmp.locate.vy = wlddt[i].y;
        tmp.locate.vz = wlddt[i].z;
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

        sprintf((char *)name, D_80097A90);
        vfree(MapModel);
        i = 0;
        MapModel =
            (u32 *)PathFileRead((char *)ImagePath, (char *)name);
        ix = (ParentingType *)(MapModel + 2);
        D_80097A70 = LoadOrnamentArchive(MapModel, &World);

        while (1)
        {
            if (!(i < D_80097A70->n))
                break;
            parent = ix[i].np;
            D_80097A70->object[i]->locate.coord.t[0] *= 10;
            D_80097A70->object[i]->locate.coord.t[1] *= 10;
            D_80097A70->object[i]->locate.coord.t[2] *= 10;

            {
                long a = D_80097A70->object[i]->locate.coord.t[0];
                long q;

                msize =
                    (parent - parent * 8) * 2;
                if (a >= 0)
                    q = a / 16000;
                else
                    q = a / 16000 - 1;
                x = q & 7;
            }
            {
                long a = D_80097A70->object[i]->locate.coord.t[1];
                long q;

                if (a >= 0)
                    q = a / 16000;
                else
                    q = a / 16000 - 1;
                y = q & 7;
            }
            {
                long a = D_80097A70->object[i]->locate.coord.t[2];
                long q;

                if (a >= 0)
                    q = a / 16000;
                else
                    q = a / 16000 - 1;
                z = q & 7;
            }

            D_80097A70->object[i]->object.attribute |= 0x400;
            UpdateOrnament(D_80097A70->object[i], 0);
            slot = (ObjectSlotType **)((z << 2) + ((x << 8) + (y << 5)));
            slot = (ObjectSlotType **)((int)slot + (int)WorldMap);
            slotman = &ModelSlot;
            model = D_80097A70->object[i];
            if (slotman->n >= slotman->max)
                AdtMessageBox(D_800120C4);
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
