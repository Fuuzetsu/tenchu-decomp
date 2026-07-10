#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PlayVoice(int id);
 *     IMAGES.C:485, 35 src lines, frame 72 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s3       int id
 *     reg   $s6       unsigned char * FileName
 *     stack sp+24     struct CdlLOC start
 *     stack sp+32     struct CdlLOC end
 *     reg   $s4       struct TVoiceTable * voice
 *     reg   $s0       unsigned char min
 *     reg   $s1       unsigned char sec
 *     reg   $s2       struct CdlLOC * loc
 *     reg   $s0       unsigned char min
 *     reg   $s1       unsigned char sec
 *
 * Globals it touches, as the original declared them:
 *     extern struct StageCharType StageChar[18];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PlayVoice", PlayVoice);

// triage: MEDIUM — 189 insns, 4 loop, 8 callees, ~0.07 to InsertConflict
// likely-relevant cookbook sections:
//   - Loops: 4 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void PlayVoice(int id)
//
// {
//   byte bVar1;
//   byte bVar2;
//   uint uVar3;
//   int iVar4;
//   byte *pbVar5;
//   byte *pbVar6;
//   undefined1 uVar7;
//   undefined *puVar8;
//   undefined *local_48 [4];
//   undefined *local_38 [4];
//   CdlLOC local_28 [2];
//   CdlLOC local_20 [2];
//
//   local_48[0] = PTR_s__TENCHU_XA_EVENT_E_XA_1_80097ca0;
//   local_48[1] = PTR_s__TENCHU_XA_EVENT_F_XA_1_80097ca4;
//   local_48[2] = PTR_s__TENCHU_XA_EVENT_I_XA_1_80097ca8;
//   local_48[3] = PTR_s__TENCHU_XA_EVENT_J_XA_1_80097cac;
//   local_38[0] = PTR_DAT_800134e0;
//   local_38[1] = PTR_DAT_800134e4;
//   local_38[2] = PTR_DAT_800134e8;
//   local_38[3] = PTR_DAT_800134ec;
//   memset(&local_28[0].minute,'\0',4);
//   memset(&local_20[0].minute,'\0',4);
//   if (id < 100) {
//     pbVar5 = local_38[(byte)PersistentState._94_1_];
//     puVar8 = local_48[(byte)PersistentState._94_1_];
//     pbVar6 = (byte *)0x0;
//     if (*pbVar5 != 0xff) {
//       uVar3 = (uint)*pbVar5;
//       do {
//         pbVar6 = pbVar5;
//         if (id == uVar3) break;
//         pbVar5 = pbVar5 + 6;
//         uVar3 = (uint)*pbVar5;
//         pbVar6 = (byte *)0x0;
//       } while (uVar3 != 0xff);
//     }
//   }
//   else {
//     if (id < 200) {
//       pbVar5 = &DAT_8008e82c;
//       id = id - 100;
//       puVar8 = PTR_s__TENCHU_XA_INTRO_XA_1_80097c98;
//     }
//     else {
//       pbVar5 = &DAT_8008e930;
//       id = id - 200;
//       puVar8 = PTR_s__TENCHU_XA_TORA_XA_1_80097c9c;
//     }
//     pbVar6 = (byte *)0x0;
//     if (*pbVar5 != 0xff) {
//       do {
//         pbVar6 = pbVar5;
//         if (id == (uint)*pbVar5) goto LAB_8004f070;
//         pbVar6 = pbVar5 + 6;
//         pbVar5 = pbVar5 + 6;
//       } while (*pbVar6 != 0xff);
//       pbVar6 = (byte *)0x0;
//     }
//   }
// LAB_8004f070:
//   if (pbVar6 == (byte *)0x0) {
//     pbVar6 = &DAT_80012cbc;
//     if (DAT_80012cbc != 0xff) {
//       uVar3 = (uint)DAT_80012cbc;
//       do {
//         if (id == uVar3) goto LAB_8004f0bc;
//         pbVar6 = pbVar6 + 6;
//         uVar3 = (uint)*pbVar6;
//       } while (uVar3 != 0xff);
//     }
//     pbVar6 = (byte *)0x0;
// LAB_8004f0bc:
//     puVar8 = PTR_s__TENCHU_XA_EVENT_E_XA_1_80097ca0;
//     if (pbVar6 == (byte *)0x0) {
//       AdtMessageBox("bad voice no %d",id);
//       CdaStop();
//       return;
//     }
//   }
//   uVar7 = PersistentState._91_1_;
//   if (0x7e < (byte)PersistentState._91_1_) {
//     uVar7 = 0x7f;
//   }
//   SsSetMVol(0x7f,0x7f);
//   FUN_8004fbf4(uVar7,uVar7);
//   bVar1 = pbVar6[2];
//   bVar2 = pbVar6[3];
//   memset(&local_28[0].minute,'\0',4);
//   local_28[0].minute = bVar1;
//   local_28[0].second = bVar2;
//   iVar4 = CdPosToInt(local_28);
//   CdIntToPos(iVar4 * 2 + 0x96,local_28);
//   bVar1 = pbVar6[4];
//   bVar2 = pbVar6[5];
//   memset(&local_20[0].minute,'\0',4);
//   local_20[0].minute = bVar1;
//   local_20[0].second = bVar2;
//   iVar4 = CdPosToInt(local_20);
//   CdIntToPos(iVar4 * 2 + 0x96,local_20);
//   iVar4 = CdaPlayXA(puVar8,local_28,local_20,pbVar6[1],0);
//   if (iVar4 == 0) {
//     AdtMessageBox("playvoice fail %s  chan %d  id %d",puVar8,(uint)pbVar6[1],id);
//   }
//   return;
// }
