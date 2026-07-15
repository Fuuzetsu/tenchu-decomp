#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct BackGround * SetupBG(struct GsIMAGE *image, short w, short h);
 *     3DCTRL.C:622, 60 src lines, frame 96 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetupBG", SetupBG);
#else

/* Complete semantic draft. Retail still differs in allocator spill shape:
 * 0x58 candidate frame / 1076 bytes versus 0x60 / 1072 bytes. */

typedef struct GsCELL
{
    u8 u;
    u8 v;
    u16 cba;
    u16 flag;
    u16 tpage;
} GsCELL;

typedef struct GsMAP
{
    u8 cellw;
    u8 cellh;
    u16 ncellw;
    u16 ncellh;
    GsCELL *base;
    u16 *index;
} GsMAP;

typedef struct GsBG
{
    u32 attribute;
    s16 x;
    s16 y;
    s16 w;
    s16 h;
    s16 scrollx;
    s16 scrolly;
    u8 r;
    u8 g;
    u8 b;
    GsMAP *map;
    s16 mx;
    s16 my;
    s16 scalex;
    s16 scaley;
    s32 rotate;
} GsBG;

typedef struct BackGround
{
    GsBG hundle;
    GsMAP map;
    GsCELL *cell;
    u32 *work;
    u16 *index;
    u16 sz;
    s16 id;
    s16 attribute;
} BackGround;

extern char D_8001109C[];
extern void SystemOut(u8 *message);
extern void *valloc(u32 size);
extern u16 GetClut(s16 x, s16 y);
extern void GsInitFixBg16(GsBG *bg, u32 *work);

BackGround *SetupBG(GsIMAGE *image, short w, short h)
{
    BackGround *bg;
    GsCELL *cell;
    short x;
    short y;
    short sy;
    short n;
    short pmode;
    short size;
    int nx;
    u32 *work;

    if (image == 0)
        SystemOut((u8 *)D_8001109C);

    bg = (BackGround *)valloc(sizeof(BackGround));
    bg->id = -1;
    bg->attribute = 0;
    memset(bg, 0, sizeof(GsBG));

    bg->hundle.w = w;
    bg->hundle.h = h;
    bg->hundle.r = bg->hundle.g = bg->hundle.b = 0x80;
    bg->hundle.map = &bg->map;
    bg->hundle.mx = w >> 1;
    bg->hundle.my = h >> 1;
    bg->hundle.scalex = bg->hundle.scaley = 0x1000;

    bg->map.cellw = bg->map.cellh = 0x10;
    bg->map.ncellw = w / bg->map.cellw;
    bg->map.ncellh = h / bg->map.cellh;

    pmode = image->pmode & 3;
    bg->hundle.attribute = pmode << 24;

    size = (short)(bg->map.ncellw * bg->map.ncellh);
    bg->index = (u16 *)valloc(size << 1);
    bg->map.index = bg->index;
    n = 0;
    if (0 < size)
    {
        do
        {
            bg->index[n] = 0xffff;
            n++;
        } while (n < size);
    }

    nx = ((u16)image->pw << (2 - pmode)) / bg->map.cellw;
    sy = (short)((u16)image->ph / bg->map.cellh);
    cell = (GsCELL *)valloc((short)nx * sy * sizeof(GsCELL));
    bg->cell = cell;
    bg->map.base = cell;

    y = 0;
    size = (1 << (8 - pmode)) - 1;
    if (0 < sy)
    {
        do
        {
            x = 0;
            if (0 < (short)nx)
            {
                do
                {
                    s16 py;
                    s16 px;

                    cell = bg->cell + y * (short)nx + x;
                    py = image->py + y * bg->map.cellh;
                    px = image->px;
                    cell->v = py;
                    cell->u = ((px << (2 - pmode)) + x * bg->map.cellw) & size;
                    cell->cba = GetClut(image->cx, image->cy);
                    cell->flag = 0;
                    cell->tpage = GetTPage(
                        pmode,
                        0,
                        px + x * (bg->map.cellw >> (2 - pmode)),
                        py);
                    x++;
                } while (x < (short)nx);
            }
            y++;
        } while (y < sy);
    }

    work = (u32 *)valloc(
        (((bg->map.ncellw + 1) * (bg->map.ncellh + 2) * 12 + 10) << 16) >> 14);
    bg->work = work;
    GsInitFixBg16(&bg->hundle, work);
    bg->sz = 0;
    return bg;
}

#endif

// triage: HARD — 268 insns, mul/div, 3 loop, 6 callees, ~0.04 to GetVectorRotation
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Removing unreachable block (ram,0x80017e70) */
// /* WARNING: Removing unreachable block (ram,0x80017e2c) */
// /* WARNING: Removing unreachable block (ram,0x80017e3c) */
// /* WARNING: Removing unreachable block (ram,0x80017e44) */
// /* WARNING: Removing unreachable block (ram,0x80017e80) */
// /* WARNING: Removing unreachable block (ram,0x80017e88) */
//
// BackGround * SetupBG(BackGround *bg,short w,short h)
//
// {
//   byte bVar1;
//   short sVar2;
//   ushort uVar3;
//   ulong uVar4;
//   u_short uVar5;
//   BackGround *bg_00;
//   ushort *puVar6;
//   GsCELL *pGVar7;
//   u_long *work;
//   int iVar8;
//   uint uVar9;
//   uint tp;
//   int iVar10;
//   ushort uVar11;
//   int iVar12;
//   ushort local_50;
//
//   if (bg == (BackGround *)0x0) {
//                     /* WARNING: Subroutine does not return */
//     SystemOut((uchar *)"NO BACKGROUND IMAGE DATA");
//   }
//   bg_00 = (BackGround *)valloc(0x48);
//   bg_00->id = -1;
//   bg_00->attribute = 0;
//   memset((uchar *)bg_00,'\0',0x24);
//   uVar4 = (bg->hundle).attribute;
//   (bg_00->map).cellh = '\x10';
//   (bg_00->hundle).b = 0x80;
//   (bg_00->hundle).g = 0x80;
//   (bg_00->hundle).r = 0x80;
//   (bg_00->hundle).scaley = 0x1000;
//   (bg_00->hundle).scalex = 0x1000;
//   (bg_00->hundle).w = w;
//   (bg_00->hundle).h = h;
//   (bg_00->hundle).map = &bg_00->map;
//   (bg_00->map).cellw = '\x10';
//   tp = (ushort)uVar4 & 3;
//   (bg_00->map).ncellw = w / 0x10;
//   (bg_00->hundle).attribute = tp << 0x18;
//   (bg_00->hundle).mx = w >> 1;
//   (bg_00->hundle).my = h >> 1;
//   (bg_00->map).ncellh = h / 0x10;
//   iVar12 = (int)(short)((w / 0x10) * (h / 0x10));
//   puVar6 = (ushort *)valloc(iVar12 << 1);
//   iVar10 = 0;
//   bg_00->index = puVar6;
//   (bg_00->map).index = puVar6;
//   if (0 < iVar12) {
//     do {
//       iVar8 = iVar10 + 1;
//       *(undefined2 *)(((iVar10 << 0x10) >> 0xf) + (int)bg_00->index) = 0xffff;
//       iVar10 = iVar8;
//     } while (iVar8 * 0x10000 >> 0x10 < iVar12);
//   }
//   uVar9 = (uint)(bg_00->map).cellw;
//   iVar10 = (uint)(ushort)(bg->hundle).w << (2 - tp & 0x1f);
//   if (uVar9 == 0) {
//     trap(0x1c00);
//   }
//   if ((uVar9 == 0xffffffff) && (iVar10 == -0x80000000)) {
//     trap(0x1800);
//   }
//   uVar3 = (bg->hundle).h;
//   bVar1 = (bg_00->map).cellh;
//   uVar11 = uVar3 / bVar1;
//   if (bVar1 == 0) {
//     trap(0x1c00);
//   }
//   if ((bVar1 == 0xffffffff) && (uVar3 == 0x80000000)) {
//     trap(0x1800);
//   }
//   iVar10 = (int)(short)(iVar10 / (int)uVar9);
//   local_50 = 0;
//   pGVar7 = (GsCELL *)valloc(iVar10 * (short)uVar11 * 8);
//   bg_00->cell = pGVar7;
//   (bg_00->map).base = pGVar7;
//   if (0 < (short)uVar11) {
//     do {
//       iVar12 = 0;
//       if (0 < iVar10) {
//         do {
//           bVar1 = (bg_00->map).cellw;
//           uVar3 = (bg->hundle).x;
//           iVar8 = (uint)(ushort)(bg->hundle).y + (int)(short)local_50 * (uint)(bg_00->map).cellh;
//           pGVar7 = bg_00->cell + (short)local_50 * iVar10 + (int)(short)iVar12;
//           pGVar7->v = (uchar)iVar8;
//           pGVar7->u = (char)(1 << (8 - tp & 0x1f)) - 1U &
//                       (char)((int)(short)uVar3 << (2 - tp & 0x1f)) + (char)iVar12 * bVar1;
//           sVar2._0_1_ = (bg->hundle).r;
//           sVar2._1_1_ = (bg->hundle).g;
//           uVar5 = GetClut((int)sVar2,(int)*(short *)&(bg->hundle).b);
//           pGVar7->cba = uVar5;
//           pGVar7->flag = 0;
//           uVar5 = GetTPage(tp,0,(int)(((uint)uVar3 +
//                                       (int)(short)iVar12 * ((int)(uint)bVar1 >> (2 - tp & 0x1f))) *
//                                      0x10000) >> 0x10,iVar8 * 0x10000 >> 0x10);
//           iVar12 = iVar12 + 1;
//           pGVar7->tpage = uVar5;
//         } while (iVar12 * 0x10000 >> 0x10 < iVar10);
//       }
//       local_50 = local_50 + 1;
//     } while ((int)((uint)local_50 << 0x10) < (int)((uint)uVar11 << 0x10));
//   }
//   work = (u_long *)
//          valloc((int)((((bg_00->map).ncellw + 1) * ((bg_00->map).ncellh + 2) * 0xc + 10) * 0x10000)
//                 >> 0xe);
//   bg_00->work = work;
//   GsInitFixBg16((GsBG *)bg_00,work);
//   bg_00->sz = 0;
//   return bg_00;
// }
