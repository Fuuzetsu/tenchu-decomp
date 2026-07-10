#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ComPad(int port, unsigned char *rxbuf);
 *     PADCMD.C:136, 100 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s3       int port
 *     param $s2       unsigned char * rxbuf
 *     reg   $s0       struct TPadPort * pad
 *     reg   $s1       int initlevel
 *     reg   $s0       int i
 *     reg   $s3       int port
 *     reg   $v1       int i
 *     reg   $v1       int i
 *     reg   $a1       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ComPad", ComPad);

// triage: MEDIUM — 120 insns, 1 loop, 5 callees, ~0.07 to GetRealPad
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ComPad(int port,uchar *rxbuf)
//
// {
//   uchar uVar1;
//   uchar uVar2;
//   ushort uVar3;
//   ushort uVar4;
//   undefined1 uVar5;
//   int iVar6;
//   ushort *puVar7;
//   int iVar8;
//
//   if (rxbuf[1] >> 4 == 8) {
//     iVar6 = 0;
//     iVar8 = 2;
//     do {
//       ComPad(port + iVar6,rxbuf + iVar8);
//       iVar6 = iVar6 + 1;
//       iVar8 = iVar8 + 8;
//     } while (iVar6 < 4);
//   }
//   else {
//     puVar7 = (ushort *)(&PadPort + (port >> 4) * 0x38 + (port & 3U) * 0xe);
//     if (*rxbuf == '\0') {
//       uVar1 = rxbuf[2];
//       uVar2 = rxbuf[3];
//       puVar7[2] = 0;
//       puVar7[1] = 0;
//       uVar4 = ~CONCAT11(uVar1,uVar2);
//       *puVar7 = uVar4;
//       uVar3 = 0x2d;
//       if (((uVar4 & 0x2000) != 0) || (uVar3 = 0xffd3, (uVar4 & 0x8000) != 0)) {
//         puVar7[1] = uVar3;
//       }
//       uVar4 = 0x2d;
//       if (((*puVar7 & 0x4000) != 0) || (uVar4 = 0xffd3, (*puVar7 & 0x1000) != 0)) {
//         puVar7[2] = uVar4;
//       }
//       if (rxbuf[1] >> 4 == 7) {
//         *(undefined1 *)((int)puVar7 + 7) = 1;
//       }
//       else {
//         *(undefined1 *)((int)puVar7 + 7) = 0;
//       }
//       iVar6 = PadInfoMode(port,2,0);
//       if (iVar6 == 0) {
//         uVar5 = (undefined1)puVar7[4];
//         *(undefined1 *)(puVar7 + 5) = 0x40;
//       }
//       else {
//         uVar5 = *(undefined1 *)((int)puVar7 + 9);
//         *(char *)(puVar7 + 5) = (char)puVar7[4];
//       }
//       *(undefined1 *)((int)puVar7 + 0xb) = uVar5;
//       iVar6 = PadGetState(port);
//       *(undefined1 *)(puVar7 + 3) = 1;
//       if (iVar6 == 1) {
//         *(undefined1 *)(puVar7 + 6) = 0;
//       }
//       if ((char)puVar7[6] == '\0') {
//         PadSetAct(port,(uchar *)(puVar7 + 5),2);
//         if (iVar6 != 2) {
//           if (iVar6 != 6) {
//             return;
//           }
//           PadSetActAlign(port,&DAT_800976f0);
//         }
//         *(undefined1 *)(puVar7 + 6) = 1;
//       }
//     }
//     else {
//       *puVar7 = 0;
//       puVar7[1] = 0;
//       puVar7[2] = 0;
//       *(undefined1 *)(puVar7 + 3) = 0;
//     }
//   }
//   return;
// }
