#include "common.h"
#include "main.exe.h"
#include "item.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawConstruction(void);
 *     WORLD.C:798, 234 src lines, frame 1920 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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
} WorldType;

extern s16 SkipFrame;
extern GsRVIEW2 ViewInfo;
extern WorldType WorldMap[8][8][8];
extern GsOT *OTablePt;
extern s32 DrawTMDmode;
extern MATRIX GsWSMATRIX;
extern char D_800120C4[];
extern char D_8001212C[];
extern char D_80012138[];
extern char D_80012148[];
extern char D_80097A98[];
extern char D_80097AA0[];

extern s32 IsVisible(s32 x, s32 y, s32 z, s32 range);
extern void DrawTMD(GsDOBJ2 *obj, GsOT *ot, s32 mode);
extern s16 GetPad(s16 no);

void DrawConstruction(void)
{
    short j;
    WorldType (*world_base)[8][8];
    short k;
    short l;
    unsigned long plimit;
    int nx;
    int ny;
    int nz;
    int ndl;
    int ndt;
    ObjectSlotType *DrawList[153];
    ObjectSlotType Slot[100];
    ObjectSlotManager SlotMan;
    int sx;
    int sy;
    int sz;
    int ex;
    int ey;
    int ez;
    ObjectSlotType *cur;
    int cell_x;
    int cell_y;
    int cell_z;
    int world_y_offset;
    int world_x_offset;
    int visible;
    ObjectSlotType **slot;
    OrnamentType *model;
    GsOT ot;
    PACKET *packet_base;

    if (SkipFrame == 1)
        return;

    if (0 <= ViewInfo.vpx)
        nx = ViewInfo.vpx / 16000;
    else
        nx = ViewInfo.vpx / 16000 - 1;
    if (0 <= ViewInfo.vpy)
        ny = ViewInfo.vpy / 16000;
    else
        ny = ViewInfo.vpy / 16000 - 1;
    if (ViewInfo.vpz < 0)
        goto negative_z;
    nz = ViewInfo.vpz / 16000;
    goto have_z;
overload:
    FntPrint(D_8001212C);
    goto draw_done;
negative_z:
    nz = ViewInfo.vpz / 16000 - 1;
have_z:

    ndl = 0;
    ndt = 0;
    sx = nx - 2;
    sy = ny - 1;
    sz = nz - 2;
    ex = nx + 2;
    ey = ny + 1;
    ez = nz + 2;
    SlotMan.max = 100;
    SlotMan.slot = Slot;
    SlotMan.n = 0;

    for (j = 0; j < 153; j++)
        DrawList[j] = 0;

    SetRotMatrix(&GsWSMATRIX);
    *(GsRVIEW2 *)TENCHU_SCRATCHPAD(0x38) = ViewInfo;

    j = sx;
scan_x:
    if (ex < (short)j)
        goto scan_done;
    cell_x = (short)j;
    k = sy;
scan_y:
    if (ey < (short)k)
        goto next_x;
    cell_y = (short)k;
    world_y_offset = (cell_y & 7) << 5;
    world_base = WorldMap;
    world_x_offset = (cell_x & 7) << 8;
    l = sz;
scan_z:
    if (ez < (short)l)
        goto next_y;
    cell_z = (short)l;
    do
    {
        do
        {
            visible = IsVisible(cell_x * 16000 + 8000,
                                cell_y * 16000 + 8000,
                                cell_z * 16000 + 8000, 0x2BC1);
        } while (0);
    } while (0);
    if (visible)
    {
        cur = *(ObjectSlotType **)(((cell_z & 7) << 2) +
                                   world_y_offset + world_x_offset +
                                   (u32)world_base);
scan_cur:
        if (cur == 0)
            goto next_z;
        if (IsVisible(cur->model->locate.coord.t[0],
                      cur->model->locate.coord.t[1] + cur->ShiftY,
                      cur->model->locate.coord.t[2], cur->ModelSize))
        {
            int bucket;
            int signed_size;

            /* The one-shot loops retain the original cc1 allocation priorities. */
            do
            {
                do
                {
                    do
                    {
                        signed_size = cur->ModelSize;
                        bucket = ((*(s32 *)TENCHU_SCRATCHPAD(0x08) -
                                   signed_size) >> 8) - 11;
                        plimit = (u16)cur->ModelSize;
                    } while (0);
                } while (0);
                if (bucket < 0)
                    bucket = 0;
                slot = (ObjectSlotType **)(bucket * sizeof(*slot) + (u32)DrawList);
                model = cur->model;

                if (SlotMan.n >= SlotMan.max)
                    AdtMessageBox(D_800120C4);
                do
                {
                    SlotMan.slot[SlotMan.n].model = model;
                    SlotMan.slot[SlotMan.n].next = *slot;
                } while (0);
                do
                {
                    SlotMan.slot[SlotMan.n].ModelSize = plimit;
                } while (0);
                SlotMan.slot[SlotMan.n].ShiftY = 0;
                *slot = &SlotMan.slot[SlotMan.n];
                ndl++;
                SlotMan.n++;
            } while (0);
        }
        ndt++;
        cur = cur->next;
        goto scan_cur;
    }
next_z:
    l++;
    goto scan_z;
next_y:
    k++;
    goto scan_y;
next_x:
    j++;
    goto scan_x;
scan_done:

    packet_base = GsGetWorkBase();
    DrawTMDmode = 0x20;
    ot = *OTablePt;
    ot.org += 0x37;

    cur = DrawList[0];
draw_near:
    if (cur == 0)
        goto draw_far_start;
    if (cur->model != 0)
    {
        GsGetLs(&cur->model->locate,
                (MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
        GsSetLsMatrix((MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
        GsSortObject4(&cur->model->object, &ot, 2,
                      (u_long *)TENCHU_SCRATCHPAD_ADDRESS);
    }
    cur = cur->next;
    goto draw_near;

draw_far_start:
    j = 1;
draw_bucket:
    if (153 <= (short)j)
        goto draw_done;
    cur = DrawList[(short)j];
draw_far:
    if (cur == 0)
        goto next_bucket;
    if (cur->model != 0)
    {
        GsGetLs(&cur->model->locate,
                (MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
        GsSetLsMatrix((MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
        DrawTMD(&cur->model->object, OTablePt, 0);
    }
    if ((u32)(GsGetWorkBase() - packet_base) > 0x6400)
        goto overload;
    cur = cur->next;
    goto draw_far;
next_bucket:
    j++;
    goto draw_bucket;

draw_done:
    if (GetPad(0) & 0x100)
    {
        FntPrint(D_80097A98);
        FntPrint(D_80012138, ndl, ndt);
        FntPrint(D_80097AA0);
        FntPrint(D_80012148, GsGetWorkBase() - packet_base);
    }
}
