#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8005adbc", FUN_8005adbc);

// triage: MEDIUM — 240 insns, 1 loop, 11 callees, ~0.05 to AddMisc
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// undefined4 FUN_8005adbc(short param_1)
//
// {
//   byte bVar1;
//   undefined4 uVar2;
//   ulong uVar3;
//   ulong *puVar4;
//   int iVar5;
//   RECT local_40;
//   GsIMAGE GStack_38;
//
//   if (param_1 == 0) {
//     uVar2 = 0;
//     if (DAT_80097d20 == (u_long *)0x0) {
//       DAT_80097d20 = (u_long *)valloc(0x8000);
//       local_40.x = 0x3c0;
//       local_40.y = 0x100;
//       local_40.w = 0x40;
//       local_40.h = 0x100;
//       StoreImage(&local_40,DAT_80097d20);
//       DrawSync(0);
//       DAT_80097d24 = FileRead("K:\\WORK\\CDIMAGE\\DEMO\\start\\card_j.txt");
//       FUN_8005b17c(0,0);
//       iVar5 = 0;
//       uVar3 = vsize((undefined *)DAT_80097d24);
//       if (0 < (int)uVar3) {
//         do {
//           bVar1 = *(byte *)((int)DAT_80097d24 + iVar5);
//           if ((bVar1 & 0x80) == 0) {
//             if ((bVar1 < 0x20) || (bVar1 == 0x5c)) {
//               *(byte *)((int)DAT_80097d24 + iVar5) = 0;
//             }
//           }
//           else {
//             iVar5 = iVar5 + 1;
//           }
//           iVar5 = iVar5 + 1;
//         } while (iVar5 < (int)uVar3);
//       }
//       puVar4 = FileRead("K:\\WORK\\CDIMAGE\\DEMO\\start\\mcard.tim");
//       GetTIMInfo(puVar4,&GStack_38);
//       LoadTIMAndFree(puVar4);
//       DAT_80097d28 = SetupSprite((Sprite3D *)0x0,&GStack_38);
//       (DAT_80097d28->sprite).y = -0x3c;
//       puVar4 = FileRead("K:\\WORK\\CDIMAGE\\DEMO\\start\\mbuttonj.tim");
//       GetTIMInfo(puVar4,&GStack_38);
//       LoadTIMAndFree(puVar4);
//       DAT_800c2d58 = SetupSprite((Sprite3D *)0x0,&GStack_38);
//       (DAT_800c2d58->sprite).h = (DAT_800c2d58->sprite).h >> 1;
//       (DAT_800c2d58->sprite).my = GStack_38.ph >> 2;
//       (DAT_800c2d58->sprite).y = 0x3c;
//       DAT_800c2d5c = SetupSprite(DAT_800c2d58,(GsIMAGE *)0x0);
//       (DAT_800c2d5c->sprite).v = (DAT_800c2d5c->sprite).v + (char)(GStack_38.ph >> 1);
//       DAT_800c2d60 = SetupSprite(DAT_800c2d58,(GsIMAGE *)0x0);
//       (DAT_800c2d60->sprite).w = ((DAT_800c2d58->sprite).w >> 1) - 0x14;
//       (DAT_800c2d60->sprite).mx = ((DAT_800c2d60->sprite).w >> 1) + 10;
//       (DAT_800c2d60->sprite).x = -0x14;
//       (DAT_800c2d60->sprite).u = (DAT_800c2d60->sprite).u + '\x0f';
//       (DAT_800c2d60->sprite).attribute = (DAT_800c2d60->sprite).attribute | 0x30000000;
//       DAT_800c2d64 = SetupSprite(DAT_800c2d58,(GsIMAGE *)0x0);
//       (DAT_800c2d64->sprite).w = ((DAT_800c2d58->sprite).w >> 1) - 10;
//       (DAT_800c2d64->sprite).mx = (DAT_800c2d64->sprite).w >> 1;
//       (DAT_800c2d64->sprite).x = 0x1c;
//       (DAT_800c2d64->sprite).u =
//            (DAT_800c2d64->sprite).u + (char)(GStack_38.pw >> 1) * '\x04' + '\n';
//       (DAT_800c2d64->sprite).attribute = (DAT_800c2d64->sprite).attribute | 0x30000000;
//       puVar4 = FileRead("K:\\WORK\\CDIMAGE\\DEMO\\start\\xtoselj.tim");
//       GetTIMInfo(puVar4,&GStack_38);
//       LoadTIMAndFree(puVar4);
//       DAT_800c2d68 = SetupSprite((Sprite3D *)0x0,&GStack_38);
//       uVar2 = 1;
//       (DAT_800c2d68->sprite).x = 2;
//       (DAT_800c2d68->sprite).y = 0x5a;
//     }
//   }
//   else {
//     local_40.x = 0x3c0;
//     local_40.y = 0x100;
//     local_40.w = 0x40;
//     local_40.h = 0x100;
//     LoadImage(&local_40,DAT_80097d20);
//     DrawSync(0);
//     vfree((undefined *)DAT_80097d20);
//     DAT_80097d20 = (u_long *)0x0;
//     vfree((undefined *)DAT_80097d24);
//     vfree((undefined *)DAT_80097d28);
//     vfree((undefined *)DAT_800c2d58);
//     vfree((undefined *)DAT_800c2d5c);
//     vfree((undefined *)DAT_800c2d60);
//     vfree((undefined *)DAT_800c2d64);
//     vfree((undefined *)DAT_800c2d68);
//     uVar2 = 0;
//   }
//   return uVar2;
// }
