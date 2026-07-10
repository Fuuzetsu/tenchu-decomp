#include "common.h"
#include "main.exe.h"

/*
 * ProcItemLightningBolt (0x800460d0) — the lightning bolt item processor.
 * mode 0: arm a 15-frame countdown, play the ready sound if the local
 * player owns it; mode 1: retarget (SearchItemTarget2), snap the item's
 * locate to the result and register a 100-unit conflict box, advance to
 * mode 2; mode 2: every third GameClock tick, drop back to mode 1 to
 * retarget again. Every mode (and the fallthrough default) then always
 * redraws the lightning bolt effect, sprays a periodic bleed/impact, and
 * ticks the countdown, disposing when it reaches zero.
 *
 * Matching notes (see also ProcItemMakibishi.c for the item-TU/collision-box
 * conventions this shares):
 *  - `pp = (VECTOR *)item->param;` before the entry mode==0xff test — the
 *    "launch" position view sits at param OFFSET 0 (proven: SearchItemTarget2/
 *    SetLightning/SetImpact all take `pp` directly). The 15-frame countdown
 *    ("count") is a DISTINCT u8 member at param offset 0x18, reached via an
 *    explicit `(u8 *)pp + 0x18` cast off the SAME pointer — not a separate
 *    named struct.
 *  - The dispatch is a real switch over {0, 1, 2} (plus the entry's separate
 *    0xff guard): expand_case builds a signed range-split tree (`slti v0,
 *    mode,2`) rather than Goshikimai's plain sequential beq/beqz, since
 *    there are 3+ cases here. The TEST order the tree picks (1, then a
 *    2-way split, then 0 within the low half, then 2 within the high half)
 *    does NOT match the case BODIES' memory order (0, then 1, then 2) —
 *    cases must still be declared ascending (0, 1, 2) in source; the tree
 *    shape is cc1's own doing, invisible from source order.
 *  - No default: falling out of all three cases (or matching none) reaches
 *    the shared "always redraw" tail directly, exactly like ProcItemMakibishi.
 *  - `item->owner == CURRENTLY_SELECTED_CHARACTER_STATE_PTR` is a DIFFERENT
 *    symbol name than CamState.Owner despite being the identical address
 *    (0x80089f00 = CamState + 0x10): this item TU names it independently,
 *    like ActionHalt/FRAMES_UNTIL_END_OF_ALERT naming per-TU.
 *  - The countdown test uses the OLD (pre-decrement) count, and the
 *    decrement is written `count + 0xff` (not `count - 1`): the literal
 *    0x00FF (not sign-extended -1) is the actual encoded immediate —
 *    verified against the raw instruction bytes, not just Ghidra's
 *    rendering, which normally simplifies genuine `-1`s.
 *  - `item->locate->locate.coord.t[N] = pos.N` (SearchItemTarget2's output)
 *    goes through a fresh item->locate reload per component, matching the
 *    established no-cache idiom (ProcItemTeleport/Kusuri).
 *  - `n = InsertConflict(...)` must be `s32`, not `s16`: a same-statement
 *    sign-extension (right after the jal) vs a point-of-use one is a
 *    scheduling-tie lever (see ProcItemMakibishi's identical fix).
 */
#include "item.h"

/* Conflict slot (Ghidra: ConflictObjectType, 0x78 bytes; see ProcItemDrop.c). */
typedef struct
{
    ModelType *model;            /* 0x00 */
    VECTOR position;             /* 0x04 */
    SVECTOR offset;              /* 0x14 */
    SVECTOR size;                /* 0x1C */
    void *common;                /* 0x24 */
    u8 result[64];                /* 0x28 */
    u8 pad[0x10];                 /* 0x68 */
} ConflictObjectType;             /* 0x78 */

extern void SearchItemTarget2(Humanoid *owner, void *rot, VECTOR *launch, VECTOR *out);
extern s16 InsertConflict(ModelType *m);
extern void SetLightning(VECTOR *pos, VECTOR *target, s32 a, s32 b, s32 c);
extern void SetImpact(VECTOR *pos, s32 a, s32 b);
extern s32 GameClock;
extern Humanoid *CURRENTLY_SELECTED_CHARACTER_STATE_PTR;
extern ConflictObjectType ConflictObject[];

void ProcItemLightningBolt(tag_TItem *item)
{
    VECTOR *pp;
    VECTOR pos;
    u8 cnt;
    s32 n;

    pp = (VECTOR *)item->param;
    if (item->mode == 0xff)
    {
        item->mode = 0;
        return;
    }
    switch (item->mode)
    {
    case 0:
        *((u8 *)pp + 0x18) = 0xf;
        item->mode = item->mode + 1;
        if (item->owner == CURRENTLY_SELECTED_CHARACTER_STATE_PTR)
        {
            SoundEx((VECTOR *)0, 0x39);
        }
        break;

    case 1:
        SearchItemTarget2(item->owner, (void *)((u8 *)item->param + 0x10), pp, &pos);
        item->locate->locate.coord.t[0] = pos.vx;
        item->locate->locate.coord.t[1] = pos.vy;
        item->locate->locate.coord.t[2] = pos.vz;
        DeleteConflict(item->locate);
        n = InsertConflict(item->locate);
        ConflictObject[n].offset.vx = 0;
        ConflictObject[n].offset.vz = 0;
        ConflictObject[n].offset.vy = 0;
        ConflictObject[n].size.vz = 100;
        ConflictObject[n].size.vy = 100;
        ConflictObject[n].size.vx = 100;
        ConflictObject[n].common = (void *)1;
        ConflictObject[n].size.pad = 1;
        item->coll_size = 100;
        item->coll_ofsY = 0;
        item->coll_mode = 1;
        item->coll_pause = 0;
        item->mode = item->mode + 1;
        break;

    case 2:
        if (GameClock == (GameClock / 3) * 3)
        {
            item->mode = 1;
        }
        break;
    }
    SetLightning(pp, (VECTOR *)item->locate->locate.coord.t, 100, 100, 200);
    if ((GameClock & 3) == 0)
    {
        SetBleeds((VECTOR *)item->locate->locate.coord.t, 200, 20, 10, 20, 0xFFFF78);
        SetImpact(pp, 0x4000, 1);
    }
    cnt = *((u8 *)pp + 0x18);
    *((u8 *)pp + 0x18) = cnt + 0xff;
    if (cnt == 0 && item->proc != 0)
    {
        item->mode = 0xff;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != 0)
        {
            AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
        }
        item->owner = 0;
        item->proc = 0;
    }
}

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcItemLightningBolt(tag_TItem *item)
//
// {
//   byte bVar1;
//   uchar uVar2;
//   short sVar3;
//   _202fake_6 *start;
//   VECTOR local_20;
//
//   bVar1 = item->mode;
//   start = &item->param;
//   if (bVar1 == 0xff) {
//     item->mode = '\0';
//   }
//   else {
//     if (bVar1 == 1) {
//       SearchItemTarget2(item->owner,&(item->param).lightningbolt.rot,(VECTOR *)&start->launch,
//                         &local_20);
//       (item->locate->locate).coord.t[0] = local_20.vx;
//       (item->locate->locate).coord.t[1] = local_20.vy;
//       (item->locate->locate).coord.t[2] = local_20.vz;
//       DeleteConflict(item->locate);
//       sVar3 = InsertConflict(item->locate);
//       ConflictObject[sVar3].offset.vx = 0;
//       ConflictObject[sVar3].offset.vz = 0;
//       ConflictObject[sVar3].offset.vy = 0;
//       ConflictObject[sVar3].size.vz = 100;
//       ConflictObject[sVar3].size.vy = 100;
//       ConflictObject[sVar3].size.vx = 100;
//       ConflictObject[sVar3].common = (undefined *)0x1;
//       ConflictObject[sVar3].size.pad = 1;
//       uVar2 = item->mode;
//       (item->collision).size = 100;
//       (item->collision).ofsY = 0;
//       (item->collision).mode = 1;
//       (item->collision).pause = 0;
//       item->mode = uVar2 + '\x01';
//     }
//     else if (bVar1 < 2) {
//       if (bVar1 == 0) {
//         (item->param).lightningbolt.count = '\x0f';
//         item->mode = item->mode + '\x01';
//         if (item->owner == CamState.Owner) {
//           SoundEx((VECTOR *)0x0,0x39);
//         }
//       }
//     }
//     else if ((bVar1 == 2) && (GameClock == (GameClock / 3) * 3)) {
//       item->mode = '\x01';
//     }
//     SetLightning((VECTOR *)&start->launch,(VECTOR *)(item->locate->locate).coord.t,100,100,200);
//     if ((GameClock & 3U) == 0) {
//       SetBleeds((short)item->locate + 0x18,200,0x14);
//       SetImpact((VECTOR *)&start->launch,0x4000,1);
//     }
//     uVar2 = (item->param).lightningbolt.count;
//     (item->param).lightningbolt.count = uVar2 + 0xff;
//     if ((uVar2 == '\0') && (item->proc != (undefined **)0x0)) {
//       item->mode = 0xff;
//       (*(code *)item->proc)(item);
//       DeleteConflict(item->locate);
//       if (item->mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//       }
//       item->owner = (Humanoid *)0x0;
//       item->proc = (undefined **)0x0;
//     }
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? AdtMessageBox(? *, s32, u8);                      /* extern */
// ? DeleteConflict(void *);                           /* extern */
// s16 InsertConflict(void *);                         /* extern */
// ? SearchItemTarget2(s32, void *, void *, s32 *);    /* extern */
// ? SetBleeds(void *, ?, s32, ?, s32, s32);           /* extern */
// ? SetImpact(void *, ?, ?);                          /* extern */
// ? SetLightning(void *, void *, ?, ?, s32);          /* extern */
// ? SoundEx(?, ?, ?);                                 /* extern */
// extern s32 CURRENTLY_SELECTED_CHARACTER_STATE_PTR;
// extern ? D_800121CC;
// extern ? ConflictObject;
// extern s32 GameClock;
//
// void ProcItemLightningBolt(void *arg0) {
//     s32 sp18;
//     ? (*temp_v0_2)(void *);
//     u8 temp_v0;
//     u8 temp_v0_3;
//     u8 temp_v1;
//     void *temp_s1;
//     void *temp_v1_2;
//
//     temp_v1 = arg0->unk54;
//     temp_s1 = arg0 + 0x20;
//     switch (temp_v1) {                              /* irregular */
//     case 0xFF:
//         arg0->unk54 = 0U;
//         return;
//     case 0x0:
//         temp_s1->unk18 = 0xFU;
//         arg0->unk54 = (u8) (arg0->unk54 + 1);
//         if (arg0->unk0 == CURRENTLY_SELECTED_CHARACTER_STATE_PTR) {
//             SoundEx(0, 0x39, 0x64);
// block_14:
//         }
//     default:
//         SetLightning(temp_s1, arg0->unk10 + 0x18, 0x64, 0x64, 0xC8);
//         if (!(GameClock & 3)) {
//             SetBleeds(arg0->unk10 + 0x18, 0xC8, 0x14, 0xA, 0x14, 0xFFFF78);
//             SetImpact(temp_s1, 0x4000, 1);
//         }
//         temp_v0 = temp_s1->unk18;
//         temp_s1->unk18 = (u8) (temp_v0 + 0xFF);
//         if (!(temp_v0 & 0xFF)) {
//             temp_v0_2 = arg0->unkC;
//             if (temp_v0_2 != NULL) {
//                 arg0->unk54 = 0xFFU;
//                 temp_v0_2(arg0);
//                 DeleteConflict(arg0->unk10);
//                 temp_v0_3 = arg0->unk54;
//                 if (temp_v0_3 != 0) {
//                     AdtMessageBox(&D_800121CC, arg0->unk8, temp_v0_3);
//                 }
//                 arg0->unk0 = 0;
//                 arg0->unkC = NULL;
//             }
//         }
//         return;
//     case 0x1:
//         SearchItemTarget2(arg0->unk0, arg0 + 0x30, temp_s1, &sp18);
//         arg0->unk10->unk18 = sp18;
//         arg0->unk10->unk1C = sp1C;
//         arg0->unk10->unk20 = sp20;
//         DeleteConflict(arg0->unk10);
//         temp_v1_2 = (InsertConflict(arg0->unk10) * 0x78) + &ConflictObject;
//         temp_v1_2->unk14 = 0;
//         temp_v1_2->unk18 = 0;
//         temp_v1_2->unk16 = 0;
//         temp_v1_2->unk20 = 0x64;
//         temp_v1_2->unk1E = 0x64;
//         temp_v1_2->unk1C = 0x64;
//         temp_v1_2->unk24 = (s32) 1U;
//         temp_v1_2->unk22 = (s16) 1U;
//         arg0->unk1C = 0x64;
//         arg0->unk1E = 0;
//         arg0->unk14 = (s32) 1U;
//         arg0->unk18 = 0;
//         arg0->unk54 = (u8) (arg0->unk54 + 1);
//         goto block_14;
//     case 0x2:
//         if (GameClock == ((GameClock / 3) * 3)) {
//             arg0->unk54 = 1U;
//         }
//         goto block_14;
//     }
// }
