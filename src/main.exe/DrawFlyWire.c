#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawFlyWire(struct tag_EffectSlot *ef);
 *     EFFECT.C:1346, 36 src lines, frame 72 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     param $s1       struct tag_EffectSlot * ef
 *     reg   $s0       struct FlyWireType * param
 *     stack sp+24     struct VECTOR pos
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawFlyWire", DrawFlyWire);

// triage: MEDIUM — 145 insns, mul/div, 4 callees, ~0.05 to EquipWeapon
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Removing unreachable block (ram,0x80036d1c) */
//
// void DrawFlyWire(tag_EffectSlot *ef)
//
// {
//   uchar uVar1;
//   int iVar2;
//   VECTOR local_30;
//   int local_20;
//   int local_1c;
//   int local_18;
//   long local_14;
//
//   uVar1 = (ef->param).flywire.mode;
//   if (uVar1 == '\0') {
//     iVar2 = (int)(ef->param).flywire.time;
//     if (iVar2 == 0) {
//       trap(0x1c00);
//     }
//     iVar2 = (uint)(ushort)(ef->param).flywire.count + 0x1000 / iVar2;
//     (ef->param).flywire.count = (short)iVar2;
//     iVar2 = iVar2 * 0x10000 >> 0x10;
//     if (iVar2 < 0x1001) {
//       SetWire((VECTOR *)&(ef->param).blood,(VECTOR *)&(ef->param).smoke.pos.vz,
//               &(ef->param).flywire.center,iVar2);
//     }
//     else {
//       (ef->param).flywire.count = 0;
//       (ef->param).flywire.mode = (ef->param).flywire.mode + '\x01';
//       SetBleeds((short)ef + 0x14,0,0x32);
//       Sound(CamState.Owner,0x31);
//     }
//   }
//   else if (uVar1 == '\x01') {
//     memset((uchar *)&local_20,'\0',0x10);
//     iVar2 = (int)(ef->param).flywire.count;
//     local_30.vx = ((ef->param).flywire.center.vx * (5 - iVar2) +
//                   (ef->param).flywire.NCenter.vx * iVar2) / 5;
//     iVar2 = (int)(ef->param).flywire.count;
//     local_30.vy = ((ef->param).flywire.center.vy * (5 - iVar2) +
//                   (ef->param).flywire.NCenter.vy * iVar2) / 5;
//     iVar2 = (int)(ef->param).flywire.count;
//     local_30.vz = ((ef->param).flywire.center.vz * (5 - iVar2) +
//                   (ef->param).flywire.NCenter.vz * iVar2) / 5;
//     local_30.pad = local_14;
//     local_20 = local_30.vx;
//     local_1c = local_30.vy;
//     local_18 = local_30.vz;
//     SetWire((VECTOR *)&(ef->param).blood,(VECTOR *)&(ef->param).smoke.pos.vz,&local_30,0x1000);
//     if (4 < (ef->param).flywire.count) {
//       ef->proc = (undefined **)0x0;
//     }
//     (ef->param).flywire.count = (ef->param).flywire.count + 1;
//   }
//   return;
// }
