#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcMiscDoor(struct tag_TMisc *m, enum TMiscMessage msg);
 *     MISC.C:294, 116 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s0       struct tag_TMisc * m
 *     param $a1       enum TMiscMessage msg
 *     reg   $s1       struct TDoor * param
 *     reg   $s2       int r
 *     reg   $a1       int type
 *     reg   $v0       int t
 *     reg   $v0       int cid
 *     reg   $v0       int dir
 *     reg   $s2       int w
 *     reg   $s0       struct ModelType * model
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct MISC__183fake DoorData[11];
 * END PSX.SYM */

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ProcMiscDoor", ProcMiscDoor);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ProcMiscDoor", run_door_spawner_action__override__prt_8004c798_2685a9f3);

// triage: MEDIUM — 269 insns, mul/div, 10 callees, ~0.08 to ProcItemMakibishi
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcMiscDoor(tag_TMisc *m,TMiscMessage msg)
//
// {
//   byte bVar1;
//   short sVar2;
//   short sVar3;
//   long lVar4;
//   int iVar5;
//   int iVar6;
//   _GsCOORDINATE2 *p_Var7;
//   int iVar8;
//   ModelType *pMVar9;
//   _181fake_23 *p_Var10;
//
//   p_Var10 = &m->param;
//   if (msg == MM_DESTROY) {
//     DeleteConflict((m->param).door.locate);
//     DisposeModel((m->param).door.locate);
//   }
//   else if (msg == MM_CREATE) {
//     iVar6 = (m->param).init.b;
//     iVar5 = (m->param).init.a;
//     if (10 < iVar6) {
//       AdtMessageBox("unknown door type %d");
//       iVar6 = 0;
//     }
//     m->mode = '\0';
//     (m->param).door.r = 0;
//     (m->param).door.type = (uchar)iVar6;
//     pMVar9 = LoadModel((ulong *)0x0);
//     lVar4 = m->x;
//     (m->param).door.locate = pMVar9;
//     (pMVar9->locate).coord.t[0] = lVar4;
//     (((m->param).door.locate)->locate).coord.t[1] = m->y;
//     (((m->param).door.locate)->locate).coord.t[2] = m->z;
//     (((m->param).door.locate)->rotate).vx = 0;
//     (((m->param).door.locate)->rotate).vy = (short)iVar5;
//     (((m->param).door.locate)->rotate).vz = 0;
//     UpdateCoordinate((m->param).door.locate);
//   }
//   else if (msg == MM_PAUSE) {
//     DeleteConflict((m->param).door.locate);
//   }
//   else if (msg == MM_RESUME) {
//     sVar2 = InsertConflict((m->param).door.locate);
//     ConflictObject[sVar2].offset.vx = 0;
//     sVar3 = DoorData[(m->param).door.type].HitSize;
//     ConflictObject[sVar2].offset.vz = 0;
//     ConflictObject[sVar2].offset.vy = (short)(-(int)sVar3 / 2);
//     sVar3 = DoorData[(m->param).door.type].HitSize;
//     ConflictObject[sVar2].common = (undefined *)0x2;
//     ConflictObject[sVar2].size.pad = 8;
//     ConflictObject[sVar2].size.vy = sVar3;
//     sVar3 = (short)(((int)sVar3 / 3) * 0x10000 >> 0xf);
//     ConflictObject[sVar2].size.vx = sVar3;
//     ConflictObject[sVar2].size.vz = sVar3;
//     (m->param).door.r = 0;
//   }
//   else {
//     if (m->mode == '\0') {
//       if (((int)((p_Var10->door).locate)->attribute & 0x8000U) != 0) {
//         sVar3 = GetConflictResult((p_Var10->door).locate,-1);
//         if (ConflictObject[sVar3].common != (undefined *)0x2) {
//           lVar4 = ratan2(ConflictObject[sVar3].position.vz -
//                          (((p_Var10->door).locate)->locate).coord.t[2],
//                          ConflictObject[sVar3].position.vx -
//                          (((p_Var10->door).locate)->locate).coord.t[0]);
//           iVar5 = lVar4 + (((p_Var10->door).locate)->rotate).vy;
//           iVar8 = iVar5 + 0x2000;
//           iVar6 = iVar8;
//           if (iVar8 < 0) {
//             iVar6 = iVar5 + 0x2fff;
//           }
//           sVar3 = 0x40;
//           if (0x800 < iVar8 + (iVar6 >> 0xc) * -0x1000) {
//             sVar3 = -0x40;
//           }
//           (m->param).door.dr = sVar3;
//           m->mode = m->mode + '\x01';
//           if ((m->param).door.r == 0) {
//             SoundEx((VECTOR *)(((p_Var10->door).locate)->locate).coord.t,0x40);
//           }
//         }
//       }
//     }
//     else if (m->mode == '\x01') {
//       iVar6 = (int)(m->param).door.r;
//       if (iVar6 < 0) {
//         iVar6 = -iVar6;
//       }
//       if (iVar6 < 0x3c0) {
//         (m->param).door.r = (m->param).door.r + (m->param).door.dr;
//       }
//       else {
//         m->mode = '\0';
//       }
//     }
//     bVar1 = (m->param).door.type;
//     iVar6 = (int)(m->param).door.r;
//     if (iVar6 < 0) {
//       iVar6 = -iVar6;
//     }
//     pMVar9 = DoorData[bVar1].Model[0];
//     iVar6 = (int)DoorData[bVar1].HitSize - (DoorData[bVar1].HitSize * iVar6) / 0x2800;
//     if (pMVar9 != (ModelType *)0xffffffff) {
//       p_Var7 = (_GsCOORDINATE2 *)(p_Var10->init).a;
//       (pMVar9->locate).coord.t[0] = -iVar6;
//       (pMVar9->locate).coord.t[1] = 0;
//       (pMVar9->locate).coord.t[2] = 0;
//       (pMVar9->locate).super = p_Var7;
//       (pMVar9->rotate).vy = (m->param).door.r;
//       UpdateCoordinate(pMVar9);
//       DrawModel(pMVar9);
//     }
//     pMVar9 = DoorData[(m->param).door.type].Model[1];
//     if (pMVar9 != (ModelType *)0xffffffff) {
//       p_Var7 = (_GsCOORDINATE2 *)(p_Var10->init).a;
//       (pMVar9->locate).coord.t[0] = iVar6;
//       (pMVar9->locate).coord.t[1] = 0;
//       (pMVar9->locate).coord.t[2] = 0;
//       (pMVar9->locate).super = p_Var7;
//       (pMVar9->rotate).vy = -(m->param).door.r;
//       UpdateCoordinate(pMVar9);
//       DrawModel(pMVar9);
//     }
//   }
//   return;
// }
