#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcMiscSnowfall(struct tag_TMisc *m, enum TMiscMessage msg);
 *     MISC.C:525, 53 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s2       struct tag_TMisc * m
 *     param $a1       enum TMiscMessage msg
 *     reg   $s4       struct TSnowfall * param
 *     reg   $s1       int i
 *     reg   $v0       int w
 *     reg   $v1       int h
 *     reg   $s0       struct SVECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcMiscSnowfall", ProcMiscSnowfall);

// triage: MEDIUM — 155 insns, mul/div, 3 callees, ~0.14 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcMiscSnowfall(tag_TMisc *m,TMiscMessage msg)
//
// {
//   int iVar1;
//   undefined4 local_40;
//   undefined4 local_3c;
//   undefined4 local_38;
//   undefined4 local_34;
//   int local_30;
//   int local_2c;
//   int local_28;
//   undefined4 local_24;
//   int local_20;
//   int local_1c;
//   int local_18;
//   undefined4 local_14;
//
//   if (msg == MM_CREATE) {
//     iVar1 = (m->param).init.b;
//     m->mode = '\0';
//     (m->param).init.b = iVar1;
//   }
//   else if ((MM_RESUME < msg) && ((GameClock & 3U) == 0)) {
//     memset((uchar *)&local_38,'\0',8);
//     iVar1 = rand();
//     local_38 = CONCAT22(local_38._2_2_,(short)iVar1 + (short)(iVar1 / 0x14) * -0x14 + -10);
//     iVar1 = rand();
//     local_38 = CONCAT22((short)iVar1 + (short)(iVar1 / 0x32) * -0x32 + 0x32,(undefined2)local_38);
//     iVar1 = rand();
//     local_34 = CONCAT22(local_34._2_2_,(short)iVar1 + (short)(iVar1 / 0x14) * -0x14 + -10);
//     local_40 = local_38;
//     local_3c = local_34;
//     memset((uchar *)&local_20,'\0',0x10);
//     iVar1 = rand();
//     local_20 = ViewInfo.vrx + -3000 + iVar1 % 6000;
//     iVar1 = rand();
//     local_1c = ViewInfo.vry + -6000 + iVar1 % 3000;
//     iVar1 = rand();
//     local_28 = ViewInfo.vrz + -3000 + iVar1 % 6000;
//     local_30 = local_20;
//     local_2c = local_1c;
//     local_24 = local_14;
//     local_18 = local_28;
//     FUN_80039160(&local_30,&local_40,0x1000,0);
//   }
//   return;
// }
