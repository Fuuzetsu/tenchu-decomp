#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "padcmd.h"
#include <psxsdk/libgpu.h>
#include "tmdfast.h"

#define N_DRAW_BUCKETS 153
#define N_DRAW_SLOTS 100
#define CONSTRUCTION_CELL_CENTER_OFFSET (CONSTRUCTION_CELL / 2)
#define CONSTRUCTION_SCAN_RADIUS_XZ 2
#define CONSTRUCTION_SCAN_RADIUS_Y 1

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

extern MATRIX GsWSMATRIX;
extern char msg_modelslot_overflow[]; /* ModelSlot Overflow */
extern char msg_overload[];
extern char fmt_objs_d[]; /* objs D%d/%d; */
extern char fmt_pk_size[];
extern char str_map[]; /* map: */
extern char str_newline_2[];

extern s32 IsVisible(s32 x, s32 y, s32 z, s32 range);
extern void DrawTMD(GsDOBJ2 *obj, GsOT *ot, s32 mode);
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
    *(GsRVIEW2 *)TENCHU_SCRATCHPAD(0x38) = ViewInfo;

    {
    WorldType(*world_base)[WORLD_MAP_AXIS_SIZE][WORLD_MAP_AXIS_SIZE];
    int cell_x;
    int cell_y;
    int cell_z;
    int world_y_offset;
    int world_x_offset;
    int visible;

    j = sx;
scan_x:
    if (ex < j)
        goto scan_done;
    cell_x = j;
    k = sy;
scan_y:
    if (ey < k)
        goto next_x;
    cell_y = k;
    world_y_offset = (cell_y & WORLD_MAP_AXIS_MASK) << 5;
    world_base = WorldMap;
    world_x_offset = (cell_x & WORLD_MAP_AXIS_MASK) << 8;
    l = sz;
scan_z:
    if (ez < l)
        goto next_y;
    cell_z = l;
    do
    {
        do
        {
            visible = IsVisible(cell_x * CONSTRUCTION_CELL + CONSTRUCTION_CELL_CENTER_OFFSET,
                                cell_y * CONSTRUCTION_CELL + CONSTRUCTION_CELL_CENTER_OFFSET,
                                cell_z * CONSTRUCTION_CELL + CONSTRUCTION_CELL_CENTER_OFFSET, 0x2BC1);
        } while (0);
    } while (0);
    if (visible)
    {
        cur = ((WorldType *)(((cell_z & WORLD_MAP_AXIS_MASK) << 2) +
                             world_y_offset + world_x_offset +
                             (u32)world_base))
                  ->top;
    scan_cur:
        if (cur == 0)
            goto next_z;
        if (IsVisible(cur->model->locate.coord.t[0],
                      cur->model->locate.coord.t[1] + cur->ShiftY,
                      cur->model->locate.coord.t[2], cur->ModelSize))
        {
            int bucket;
            int signed_size;
            ObjectSlotType **slot;
            OrnamentType *model;

            /* The remaining do/while (0) layers are allocation weight for
             * three DISTINCT races (measured 2026-08-31): the IsVisible pair
             * feeds cell_x/cell_y against j (the visibility intruder, which
             * also demands $s6 reuse across three disjoint roles - fission
             * cannot co-color it); this outer wrapper and the model/next
             * store wrapper feed the slot corridor. The two former inner
             * layers and the ModelSize store wrapper fell to the plimit
             * consumer identity below. */
            do
            {
                signed_size = cur->ModelSize;
                /* IsVisible left its view-space vector in the
                 * scratchpad; +0x08 is that vector's z. */
                bucket = ((*(s32 *)TENCHU_SCRATCHPAD(0x08) -
                           signed_size) >>
                          8) -
                         11;
                plimit = (u16)cur->ModelSize;
                if (bucket < 0)
                    bucket = 0;
                /* Offset spelling: byte-required (indexing flips the addu; measured). */
                slot = (ObjectSlotType **)(bucket * sizeof(*slot) + (u32)DrawList);
                model = cur->model;

                if (SlotMan.n >= SlotMan.max)
                    AdtMessageBox(msg_modelslot_overflow);
                do
                {
                    SlotMan.slot[SlotMan.n].model = model;
                    SlotMan.slot[SlotMan.n].next = *slot;
                } while (0);
                /* Unsigned identity folded after flow: +2 counted plimit refs
                 * rank it between slot and model for $s2 (allocation
                 * staging). */
                SlotMan.slot[SlotMan.n].ModelSize = (plimit + plimit) - plimit;
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
    }
    {
    GsOT ot;
    PACKET *packet_base;

scan_done:
    packet_base = GsGetWorkBase();
    DrawTMDmode = TMD_BANK_FOG;
    ot = *OTablePt;
    ot.org += 0x37; /* bias the local OT into the global table's depth window */

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
    if (j >= N_DRAW_BUCKETS)
        goto draw_done;
    cur = DrawList[j];
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
    if ((u32)(GsGetWorkBase() - packet_base) > 0x6400) /* per-frame construction packet budget */
        goto overload;
    cur = cur->next;
    goto draw_far;
next_bucket:
    j++;
    goto draw_bucket;

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
