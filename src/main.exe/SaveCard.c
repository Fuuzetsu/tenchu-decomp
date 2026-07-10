#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SaveCard(int target, unsigned char *name, void *mem, long size);
 *     MEMCARD.C:157, 38 src lines, frame 8464 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
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
 *     param $a0       int target
 *     param $s5       unsigned char * name
 *     param $s6       void * mem
 *     param $s7       long size
 *     stack sp+24     unsigned char [200] fn
 *     stack sp+8416   long cmd
 *     stack sp+8420   long result
 *     reg   $s4       long chan
 *     stack sp+224    unsigned char [8192] block
 *     reg   $s2       struct TCardHeader * hd
 *     reg   $s3       void * data
 *     reg   $s2       struct TCardHeader * hd
 *     reg   $t1       unsigned char * icon3
 *     reg   $s1       unsigned char * icon2
 *     reg   $s0       unsigned char * icon1
 * END PSX.SYM */

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SaveCard", SaveCard);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SaveCard", FUN_80056a3c__override__prt_80056d4c_aee7b64a);

// triage: HARD — 239 insns, 6 loop, frame 0x2110, 7 callees, ~0.09 to DebugMenuItemSet
// likely-relevant cookbook sections:
//   - Loops: 6 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Stack objects: 0x2110 frame — buffer casts / spills

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short SaveCard(int target,uchar *name,undefined *mem,long size)
//
// {
//   ulong *puVar1;
//   ulong *puVar2;
//   ulong *puVar3;
//   ulong *puVar4;
//   ulong *puVar5;
//   ulong uVar6;
//   ulong uVar7;
//   ulong uVar8;
//   short in_stack_00000010;
//   char acStack_20f8 [200];
//   undefined1 local_2030;
//   undefined1 local_202f;
//   undefined1 local_202e;
//   undefined1 local_202d;
//   uchar auStack_202c [64];
//   uchar auStack_1fec [28];
//   ulong local_1fd0;
//   ulong local_1fcc;
//   ulong local_1fc8;
//   ulong local_1fc4;
//   ulong local_1fc0;
//   ulong local_1fbc;
//   ulong local_1fb8;
//   ulong local_1fb4;
//   ulong local_1fb0 [32];
//   ulong local_1f30 [32];
//   ulong local_1eb0 [32];
//   uchar auStack_1e30 [7680];
//   long lStack_30;
//   long local_2c;
//
//   local_2030 = 0x53;
//   local_202f = 0x43;
//   local_202e = 0x13;
//   local_202d = 1;
//   memset(auStack_202c,'\0',0x40);
//   sprintf((char *)auStack_202c,&DAT_80013be4);
//   memset(auStack_1fec,'\0',0x1c);
//   puVar1 = GetArcData(0x16);
//   puVar2 = GetArcData(0x17);
//   puVar3 = GetArcData(0x18);
//   puVar5 = local_1fb0;
//   puVar4 = puVar1 + 0x10;
//   local_1fd0 = puVar1[5];
//   local_1fcc = puVar1[6];
//   local_1fc8 = puVar1[7];
//   local_1fc4 = puVar1[8];
//   local_1fc0 = puVar1[9];
//   local_1fbc = puVar1[10];
//   local_1fb8 = puVar1[0xb];
//   local_1fb4 = puVar1[0xc];
//   if (((uint)puVar4 & 3) == 0) {
//     do {
//       uVar6 = puVar4[1];
//       uVar7 = puVar4[2];
//       uVar8 = puVar4[3];
//       *puVar5 = *puVar4;
//       puVar5[1] = uVar6;
//       puVar5[2] = uVar7;
//       puVar5[3] = uVar8;
//       puVar4 = puVar4 + 4;
//       puVar5 = puVar5 + 4;
//     } while (puVar4 != puVar1 + 0x30);
//   }
//   else {
//     do {
//       uVar6 = puVar4[1];
//       uVar7 = puVar4[2];
//       uVar8 = puVar4[3];
//       *puVar5 = *puVar4;
//       puVar5[1] = uVar6;
//       puVar5[2] = uVar7;
//       puVar5[3] = uVar8;
//       puVar4 = puVar4 + 4;
//       puVar5 = puVar5 + 4;
//     } while (puVar4 != puVar1 + 0x30);
//   }
//   puVar4 = local_1f30;
//   puVar1 = puVar2 + 0x10;
//   if (((uint)puVar1 & 3) == 0) {
//     do {
//       uVar6 = puVar1[1];
//       uVar7 = puVar1[2];
//       uVar8 = puVar1[3];
//       *puVar4 = *puVar1;
//       puVar4[1] = uVar6;
//       puVar4[2] = uVar7;
//       puVar4[3] = uVar8;
//       puVar1 = puVar1 + 4;
//       puVar4 = puVar4 + 4;
//     } while (puVar1 != puVar2 + 0x30);
//   }
//   else {
//     do {
//       uVar6 = puVar1[1];
//       uVar7 = puVar1[2];
//       uVar8 = puVar1[3];
//       *puVar4 = *puVar1;
//       puVar4[1] = uVar6;
//       puVar4[2] = uVar7;
//       puVar4[3] = uVar8;
//       puVar1 = puVar1 + 4;
//       puVar4 = puVar4 + 4;
//     } while (puVar1 != puVar2 + 0x30);
//   }
//   puVar2 = local_1eb0;
//   puVar1 = puVar3 + 0x10;
//   if (((uint)puVar1 & 3) == 0) {
//     do {
//       uVar6 = puVar1[1];
//       uVar7 = puVar1[2];
//       uVar8 = puVar1[3];
//       *puVar2 = *puVar1;
//       puVar2[1] = uVar6;
//       puVar2[2] = uVar7;
//       puVar2[3] = uVar8;
//       puVar1 = puVar1 + 4;
//       puVar2 = puVar2 + 4;
//     } while (puVar1 != puVar3 + 0x30);
//   }
//   else {
//     do {
//       uVar6 = puVar1[1];
//       uVar7 = puVar1[2];
//       uVar8 = puVar1[3];
//       *puVar2 = *puVar1;
//       puVar2[1] = uVar6;
//       puVar2[2] = uVar7;
//       puVar2[3] = uVar8;
//       puVar1 = puVar1 + 4;
//       puVar2 = puVar2 + 4;
//     } while (puVar1 != puVar3 + 0x30);
//   }
//   sprintf(acStack_20f8,&DAT_80097d08,PTR_s_BISLPS_01901_80097d04,name);
//   local_2c = MemCardCreateFile(0,acStack_20f8,1);
//   if (((local_2c == 0) || (local_2c == 6)) && (in_stack_00000010 != 0)) {
//     memcpy(auStack_1e30,mem,size);
//     local_2c = MemCardWriteFile(0,acStack_20f8,(ulong *)&local_2030,0,0x2000);
//     MemCardSync(0,&lStack_30,&local_2c);
//   }
//   return (short)local_2c;
// }
