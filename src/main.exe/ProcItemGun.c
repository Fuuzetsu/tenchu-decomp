#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemGun(struct tag_TItem *item);
 *     ITEM.C:2938, 72 src lines, frame 80 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s1       struct tag_TItem * item
 *     reg   $s2       struct param_gun * param
 *     stack sp+24     struct SVECTOR vec
 *     stack sp+32     struct VECTOR target
 *     reg   $s3       struct Humanoid * IsHuman
 *     reg   $s1       struct tag_TItem * item
 *     reg   $v0       int t
 *     stack sp+48     struct SVECTOR vec
 *     reg   $s1       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcItemGun", ProcItemGun);

// triage: MEDIUM — 203 insns, indirect-call, 10 callees, ~0.22 to ProcItemLightningBolt
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcItemGun(tag_TItem *item)
//
// {
//   byte bVar1;
//   short sVar2;
//   ModelArchiveType *pMVar3;
//   Humanoid *pHVar4;
//   SVECTOR local_40;
//   VECTOR local_38;
//   SVECTOR local_28;
//   int local_20;
//   int local_1c;
//
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//     return;
//   }
//   bVar1 = item->mode;
//   if (bVar1 == 1) {
//     GetVectorRotation((VECTOR *)(item->locate->locate).coord.t,(VECTOR *)&(item->param).launch,
//                       &local_20,&local_1c);
//     local_40._4_4_ = (uint)(ushort)local_40.pad << 0x10;
//     pHVar4 = SearchItemTarget2(item->owner,&local_40,(VECTOR *)(item->locate->locate).coord.t,
//                                &local_38);
//     (item->locate->locate).coord.t[0] = local_38.vx;
//     (item->locate->locate).coord.t[1] = local_38.vy;
//     (item->locate->locate).coord.t[2] = local_38.vz;
//     DeleteConflict(item->locate);
//     sVar2 = InsertConflict(item->locate);
//     ConflictObject[sVar2].offset.vx = 0;
//     ConflictObject[sVar2].offset.vz = 0;
//     ConflictObject[sVar2].offset.vy = 0;
//     ConflictObject[sVar2].size.vz = 100;
//     ConflictObject[sVar2].size.vy = 100;
//     ConflictObject[sVar2].size.vx = 100;
//     ConflictObject[sVar2].common = (undefined *)0x1;
//     ConflictObject[sVar2].size.pad = 1;
//     (item->collision).size = 100;
//     (item->collision).ofsY = 0;
//     (item->collision).mode = 1;
//     (item->collision).pause = 0;
//     local_28.vx = (short)DAT_80097b14;
//     local_28.vy = DAT_80097b14._2_2_;
//     local_28.vz = (short)DAT_80097b18;
//     local_28.pad = DAT_80097b18._2_2_;
//     pMVar3 = item->owner->model;
//     RotateVectorS(&local_28,(int)(pMVar3->rotate).vx,(int)(pMVar3->rotate).vy,0);
//     if (pHVar4 == (Humanoid *)0x0) {
//       SetImpact(&local_38,0x4000,0);
//       SetBleedsDir(&local_38,&local_28,100,0xf);
//       sVar2 = 0x35;
//     }
//     else {
//       SetImpact(&local_38,0x6000,0);
//       SetBleedsDir(&local_38,&local_28,100,0xf);
//       sVar2 = 0x34;
//     }
//     SoundEx(&local_38,sVar2);
//   }
//   else {
//     if (1 < bVar1) {
//       if (bVar1 != 2) {
//         return;
//       }
//       if (item->proc == (undefined **)0x0) {
//         return;
//       }
//       item->mode = 0xff;
//       (*(code *)item->proc)(item);
//       DeleteConflict(item->locate);
//       if (item->mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//       }
//       item->owner = (Humanoid *)0x0;
//       item->proc = (undefined **)0x0;
//       return;
//     }
//     if (bVar1 != 0) {
//       return;
//     }
//     local_40._0_4_ = DAT_80097b0c;
//     local_40._4_4_ = DAT_80097b10;
//     pMVar3 = item->owner->model;
//     RotateVectorS(&local_40,(int)(pMVar3->rotate).vx,(int)(pMVar3->rotate).vy,0);
//     SetImpact((VECTOR *)(item->locate->locate).coord.t,0x2000,0);
//     SetBleeds((short)item->locate + 0x18,100,10);
//   }
//   item->mode = item->mode + '\x01';
//   return;
// }
