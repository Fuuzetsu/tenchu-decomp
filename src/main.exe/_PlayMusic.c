#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void _PlayMusic(int MusicNo, int mode);
 *     IMAGES.C:438, 37 src lines, frame 264 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s3       int MusicNo
 *     param $s4       int mode
 *     stack sp+24     unsigned char [200] fname
 *     stack sp+224    struct CdlLOC start
 *     stack sp+232    struct CdlLOC end
 *     reg   $s2       struct TMusicTable * music
 *     reg   $s0       unsigned char min
 *     reg   $s1       unsigned char sec
 * END PSX.SYM */

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/_PlayMusic", _PlayMusic);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/_PlayMusic", play_stage_music__override__prt_8004ed94_2feb0bc8);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/_PlayMusic", play_stage_music__override__prt_8004edf4_f3434af);

// triage: MEDIUM — 100 insns, 10 callees, ~0.08 to AddXG4
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void _PlayMusic(int MusicNo,int mode)
//
// {
//   byte bVar1;
//   byte bVar2;
//   u_char uVar3;
//   u_char uVar4;
//   int iVar5;
//   char acStack_f0 [200];
//   CdlLOC local_28 [2];
//   CdlLOC local_20 [2];
//
//   if ((MusicNo < 0) || (MusicNo - 0x3dU < 0x27)) {
//     AdtMessageBox("bad music no",MusicNo);
//     CdaStop();
//   }
//   else if (MusicNo < 0x13) {
//     sprintf(acStack_f0,"\\TENCHU\\XA\\%s;1",MusicTable[MusicNo].file);
//     SsSetMVol(0x7f,0x7f);
//     FUN_8004fbf4(PersistentState._90_1_,PersistentState._90_1_);
//     bVar1 = MusicTable[MusicNo].min;
//     bVar2 = MusicTable[MusicNo].sec;
//     memset(&local_28[0].minute,'\0',4);
//     local_28[0].minute = bVar1;
//     local_28[0].second = bVar2;
//     iVar5 = CdPosToInt(local_28);
//     CdIntToPos(iVar5 * 2 + 0x96,local_28);
//     uVar3 = MusicTable[MusicNo].field4_0x7;
//     uVar4 = MusicTable[MusicNo].field5_0x8;
//     memset(&local_20[0].minute,'\0',4);
//     local_20[0].minute = uVar3;
//     local_20[0].second = uVar4;
//     iVar5 = CdPosToInt(local_20);
//     CdIntToPos(iVar5 * 2 + 0x96,local_20);
//     iVar5 = CdaPlayXA(acStack_f0,local_28,local_20,MusicTable[MusicNo].channel,(int)(short)mode);
//     if (iVar5 == 0) {
//       AdtMessageBox("playmusic fail %s  chan %d  id %d",acStack_f0,(uint)MusicTable[MusicNo].channel
//                     ,MusicNo);
//     }
//   }
//   else {
//     iVar5 = MusicNo + 0x52;
//     if (99 < MusicNo) {
//       iVar5 = MusicNo + 100;
//     }
//     PlayVoice(iVar5);
//   }
//   return;
// }
