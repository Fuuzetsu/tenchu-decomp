#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemKawarimi(struct tag_TItem *item);
 *     ITEM.C:1571, 35 src lines, frame 80 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     param $s0       struct tag_TItem * item
 *     reg   $s4       struct param_drop * param
 *     reg   $s2       int i
 *     stack sp+16     struct VECTOR pos
 *     stack sp+32     struct SVECTOR vec
 *     reg   $s0       struct tag_TItem * item
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcItemKawarimi", ProcItemKawarimi);

// triage: MEDIUM — 196 insns, mul/div, indirect-call, 5 callees, ~0.35 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcItemKawarimi(tag_TItem *item)
//
// {
//   byte bVar1;
//   int iVar2;
//   int iVar3;
//   int iVar4;
//   VECTOR local_40;
//   SVECTOR local_30;
//   SVECTOR local_28;
//
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//     return;
//   }
//   bVar1 = item->mode;
//   if (bVar1 == 1) {
//     for (iVar4 = 0; iVar4 < 0x14; iVar4 = iVar4 + 1) {
//       memset((uchar *)&local_30,'\0',0x10);
//       iVar2 = rand();
//       local_30._0_4_ = (item->owner->model->locate).coord.t[0] + -500 + iVar2 % 1000;
//       iVar2 = rand();
//       local_30._4_4_ = (item->owner->model->locate).coord.t[1] + -0x4b0 + iVar2 % 1000;
//       iVar2 = rand();
//       local_40.vz = (item->owner->model->locate).coord.t[2] + -500 + iVar2 % 1000;
//       local_40.vx._0_2_ = local_30.vx;
//       local_40.vx._2_2_ = local_30.vy;
//       local_40.vy._0_2_ = local_30.vz;
//       local_40.vy._2_2_ = local_30.pad;
//       local_40.pad._0_2_ = local_28.vz;
//       local_40.pad._2_2_ = local_28.pad;
//       local_28._0_4_ = local_40.vz;
//       memset((uchar *)&local_28,'\0',8);
//       iVar2 = rand();
//       local_30.vy = (short)iVar2 + (short)(iVar2 / 10) * -10 + -0x1e;
//       local_28.vy = local_30.vy;
//       local_30.vx = local_28.vx;
//       local_30.vz = local_28.vz;
//       local_30.pad = local_28.pad;
//       iVar3 = rand();
//       iVar2 = iVar3;
//       if (iVar3 < 0) {
//         iVar2 = iVar3 + 0xf;
//       }
//       SetBleed(&local_40,&local_30,iVar3 + (iVar2 >> 4) * -0x10 + 0xf,0x64c8dc);
//     }
//     bVar1 = (item->param).smoke.count + 1;
//     (item->param).smoke.count = bVar1;
//     if (bVar1 < 0x1f) {
//       return;
//     }
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
//     (item->param).smoke.count = '\0';
//   }
//   item->mode = item->mode + '\x01';
//   return;
// }
