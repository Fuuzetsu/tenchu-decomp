#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PutItemIcon(int ItemID, short x, short y, short scale);
 *     INFOVIEW.C:349, 7 src lines, frame 24 bytes, saved-reg mask 0x80000000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a0       int ItemID
 *     param $a1       short x
 *     param $a2       short y
 *     param $a3       short scale
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * PutItemIcon (0x8004c0e4) — position/scale the item-menu icon sprite for
 * ItemID and sort it into the GsOT. Same field-set as PutItemCursor
 * (x/y/scalex/scaley + GsSortSprite), but here the GsSPRITE lives embedded
 * in a per-item slot (D_8008E5BC[ItemID], the same model-pointer array the
 * Req/Proc item family indexes by it->type for `it->model`) rather than in
 * a standalone global: Ghidra's raw type extends item.h's Sprite3D (0x68
 * bytes, ends right where this GsSPRITE starts) with an embedded `sprite`
 * field at +0x68 — local wrapper struct reuses item.h's Sprite3D instead of
 * redefining it.
 */
typedef struct
{
    Sprite3D model;
    GsSPRITE sprite;
} ItemIconType;

extern ItemIconType *D_8008E5BC[];
extern GsOT *OTablePt;
extern void GsSortSprite(GsSPRITE *sp, GsOT *ot, int pri);

void PutItemIcon(s32 ItemID, short x, short y, short scale)
{
    GsSPRITE *spr = &D_8008E5BC[ItemID]->sprite;

    spr->x = x;
    spr->y = y;
    spr->scalex = scale;
    spr->scaley = scale;
    GsSortSprite(spr, OTablePt, 0);
}

// triage: TRIVIAL — 21 insns, 1 callees, ~0.06 to PutItemCursor

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void PutItemIcon(int ItemID,short x,short y,short scale)
//
// {
//   GsOT *ot;
//   int iVar1;
//
//   iVar1 = (&DAT_8008e5bc)[ItemID];
//   *(short *)(iVar1 + 0x6c) = x;
//   ot = OTablePt;
//   *(short *)(iVar1 + 0x6e) = y;
//   *(short *)(iVar1 + 0x84) = scale;
//   *(short *)(iVar1 + 0x86) = scale;
//   GsSortSprite((GsSPRITE *)(iVar1 + 0x68),ot,0);
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? GsSortSprite(void *, s32, ?);                     /* extern */
// extern ? D_8008E5BC;
// extern s32 OTablePt;
//
// void PutItemIcon(s32 arg0, s16 arg1, s16 arg2, s16 arg3) {
//     void *temp_a0;
//
//     temp_a0 = *((arg0 * 4) + &D_8008E5BC) + 0x68;
//     temp_a0->unk4 = arg1;
//     temp_a0->unk6 = arg2;
//     temp_a0->unk1C = arg3;
//     temp_a0->unk1E = arg3;
//     GsSortSprite(temp_a0, OTablePt, 0);
// }
