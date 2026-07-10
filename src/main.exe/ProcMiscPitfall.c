#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcMiscPitfall(struct tag_TMisc *m, enum TMiscMessage msg);
 *     MISC.C:414, 106 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s1       struct tag_TMisc * m
 *     param $a1       enum TMiscMessage msg
 *     reg   $s2       struct TPitfall * param
 *     reg   $s0       struct ModelType * model
 *     reg   $s1       short w
 *     reg   $s0       int r
 *     reg   $a1       int type
 *     reg   $v0       int t
 *
 * Globals it touches, as the original declared them:
 *     extern struct MISC__184fake PitfallData[2];
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcMiscPitfall", ProcMiscPitfall);

// triage: MEDIUM — 217 insns, mul/div, 9 callees, ~0.07 to ProcItemLightningBolt
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcMiscPitfall(tag_TMisc *m,TMiscMessage msg)
//
// {
//   byte bVar1;
//   short sVar2;
//   short sVar3;
//   long lVar4;
//   uint uVar5;
//   _GsCOORDINATE2 *p_Var6;
//   int iVar7;
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
//     iVar7 = (m->param).init.b;
//     iVar8 = (m->param).init.a;
//     if (2 < iVar7) {
//       AdtMessageBox("unknown pitfall type %d");
//       iVar7 = 0;
//     }
//     m->mode = '\0';
//     (m->param).door.r = 0;
//     (m->param).pitfall.type = (uchar)iVar7;
//     pMVar9 = LoadModel((ulong *)0x0);
//     lVar4 = m->x;
//     (m->param).door.locate = pMVar9;
//     (pMVar9->locate).coord.t[0] = lVar4;
//     (((m->param).door.locate)->locate).coord.t[1] = m->y;
//     (((m->param).door.locate)->locate).coord.t[2] = m->z;
//     (((m->param).door.locate)->rotate).vx = 0;
//     (((m->param).door.locate)->rotate).vy = (short)iVar8;
//     (((m->param).door.locate)->rotate).vz = 0;
//     UpdateCoordinate((m->param).door.locate);
//   }
//   else if (msg == MM_PAUSE) {
//     DeleteConflict((m->param).door.locate);
//   }
//   else if (msg == MM_RESUME) {
//     sVar3 = PitfallData[(m->param).pitfall.type].HitSize;
//     sVar2 = InsertConflict((m->param).door.locate);
//     ConflictObject[sVar2].offset.vx = 0;
//     ConflictObject[sVar2].offset.vy = 0;
//     ConflictObject[sVar2].offset.vz = 0;
//     ConflictObject[sVar2].common = (undefined *)0x2;
//     ConflictObject[sVar2].size.pad = 8;
//     ConflictObject[sVar2].size.vx = sVar3;
//     sVar3 = (short)(((int)sVar3 / 3) * 0x10000 >> 0xf);
//     ConflictObject[sVar2].size.vz = sVar3;
//     ConflictObject[sVar2].size.vy = sVar3;
//   }
//   else {
//     bVar1 = m->mode;
//     if (bVar1 == 1) {
//       sVar3 = (m->param).door.r + 0xaa;
//       (m->param).door.r = sVar3;
//       if (0x3ff < sVar3) {
//         (m->param).door.r = 0x400;
//         m->mode = m->mode + '\x01';
//       }
//     }
//     else if ((((bVar1 < 2) && (bVar1 == 0)) &&
//              (((int)((p_Var10->door).locate)->attribute & 0x8000U) != 0)) &&
//             (sVar3 = GetConflictResult((p_Var10->door).locate,-1),
//             ConflictObject[sVar3].common != (undefined *)0x2)) {
//       m->mode = m->mode + '\x01';
//       SoundEx((VECTOR *)(((p_Var10->door).locate)->locate).coord.t,0x40);
//     }
//     uVar5 = (uint)(m->param).pitfall.type;
//     pMVar9 = PitfallData[uVar5].Model[0];
//     sVar3 = PitfallData[uVar5].HitSize;
//     if (pMVar9 != (ModelType *)0xffffffff) {
//       p_Var6 = (_GsCOORDINATE2 *)(p_Var10->init).a;
//       (pMVar9->locate).coord.t[0] = -(int)sVar3;
//       (pMVar9->locate).coord.t[1] = 0;
//       (pMVar9->locate).coord.t[2] = 0;
//       (pMVar9->locate).super = p_Var6;
//       (pMVar9->rotate).vz = (m->param).door.r;
//       UpdateCoordinate(pMVar9);
//       DrawModel(pMVar9);
//       uVar5 = (uint)(m->param).pitfall.type;
//     }
//     pMVar9 = PitfallData[uVar5].Model[1];
//     if (pMVar9 != (ModelType *)0xffffffff) {
//       p_Var6 = (_GsCOORDINATE2 *)(p_Var10->init).a;
//       (pMVar9->locate).coord.t[0] = (int)sVar3;
//       (pMVar9->locate).coord.t[1] = 0;
//       (pMVar9->locate).coord.t[2] = 0;
//       (pMVar9->locate).super = p_Var6;
//       (pMVar9->rotate).vz = -(m->param).door.r;
//       UpdateCoordinate(pMVar9);
//       DrawModel(pMVar9);
//     }
//   }
//   return;
// }
